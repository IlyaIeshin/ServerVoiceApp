// VoiceSfuManager.cpp
#include "voicesfumanager.h"
using json = nlohmann::json;
using namespace rtc;

VoiceSfuManager& VoiceSfuManager::instance() {
    // Инициализация логгера (вызываем один раз)
    static bool loggerInitialized = false;
    if (!loggerInitialized) {
        rtc::InitLogger(rtc::LogLevel::Warning); // или Verbose для отладки
        loggerInitialized = true;
    }
    static VoiceSfuManager instance;
    return instance;
}

void VoiceSfuManager::handleSignal(const std::string& chId,
                                   crow::websocket::connection* c,
                                   const json& msg)
{
    std::lock_guard<std::mutex> lk(mtx_);

    std::cout << "\n[SFU] ==== Incoming signal ====\n";
    std::cout << "[SFU] channel_id: " << chId << "\n";
    std::cout << "[SFU] raw message: " << msg.dump() << "\n";

    auto& room = rooms_[chId];
    auto& cli  = room.clients[c];

    std::cout << "[SFU] clients in room '" << chId << "': " << room.clients.size() << "\n";

    // Если для данного соединения ещё нет PeerConnection – создаём
    if (!cli.pc) {
        std::cout << "[SFU] Creating PeerConnection for new client in room '" << chId << "'\n";
        // Конфигурация PeerConnection
        Configuration config;
        config.iceServers = {{"stun:stun.l.google.com:19302"}};
        cli.pc = std::make_shared<PeerConnection>(config);

        cli.pc->onLocalDescription([=](Description desc) {
            std::string sdpType = desc.typeString(); // "offer" или "answer"
            std::cout << "[SFU] send " << sdpType << " to client\n";
            std::cout << "[SFU] SDP:\n" << std::string(desc) << "\n";
            json out{{"type","voice-signal"},
                     {"command", sdpType},
                     {"payload", {{"channel_id",chId},{"sdp",std::string(desc)}}}};
            c->send_text(out.dump());
        });

        // Callback: локальный ICE-кандидат
        cli.pc->onLocalCandidate([=](Candidate cand) {
            std::cout << "[SFU] send ICE candidate to client (" << chId << "): "
                      << cand.candidate() << "\n";
            json out;
            out["type"] = "voice-signal";
            out["command"] = "ice";
            out["payload"] = {
                {"channel_id", chId},
                {"candidate", cand.candidate()}
            };
            c->send_text(out.dump());
        });

        // Callback: входящий медиатрек от данного клиента
        cli.pc->onTrack([&, c](std::shared_ptr<Track> inTrack) {
            std::cout << "[SFU] onTrack: incoming from client " << c << "\n";
            std::cout << "[SFU] current clients in room '" << chId << "': " << room.clients.size() << "\n";
            cli.uplink = inTrack; // сохраняем uplink-трек

            // Настраиваем депакетизатор для Opus
            auto desc = inTrack->description();
            inTrack->setMediaHandler(std::make_shared<rtc::OpusRtpDepacketizer>());
            inTrack->setDescription(desc);

            // Обработка закрытия входящего трека
            inTrack->onClosed([&]() {
                std::cout << "[SFU] uplink track from client closed\n";
                // Удаляем связанные исходящие треки у всех клиентов
                for (auto& [otherConn, otherCli] : room.clients) {
                    otherCli.downTracks.erase(inTrack.get());
                }
            });

            // Callback на каждый полученный аудиофрейм
            inTrack->onFrame([&, inTrack, c](const rtc::binary& frame,
                                             const rtc::FrameInfo& fi)
             {
                 std::cout << "[SERVER] Received audio frame (ts=" << fi.timestamp << ", size=" << frame.size() << " bytes)\n";

                 // Пересылаем кадр каждому другому участнику комнаты
                 for (auto& [otherConn, otherCli] : room.clients) {
                     if (otherConn == c || !otherCli.pc) continue; // себе не шлём

                     std::cout << "[SFU] sending audio frame to client " << otherConn << " ("
                               << frame.size() << " bytes, ts=" << fi.timestamp << ")\n";

                     // Для каждого получателя создаём или получаем свой downTrack
                     auto& downTrack = otherCli.downTracks[inTrack.get()];
                     if (!downTrack) {
                         // Создаём аудио-трек для передачи этому клиенту
                         Description::Audio outDesc("forward-audio", Description::Direction::SendOnly);
                         outDesc.addOpusCodec(111);  // поддерживаем Opus, PT=111
                         std::cout << "[TEST]1111111" << std::endl;
                         downTrack = otherCli.pc->addTrack(outDesc);
                         std::cout << "[TEST]2222222" << std::endl;

                         otherCli.pc->createOffer();

                         downTrack->onOpen([=]() {
                             std::cout << "[SFU] downTrack opened for a receiver\n";
                         });
                     }

                     // Отправляем аудио-фрейм получателю
                     if (downTrack->isOpen()) {
                         downTrack->sendFrame(frame, fi);
                     } else {
                         //std::cout << "[SFU] warning: try sendFrame while track not open\n";
                         downTrack->sendFrame(frame, fi);
                     }
                 }
             });
        }); // end onTrack
    } // if !cli.pc

    // Обработка входящего сигналинга (SDP и ICE) от клиента
    std::string cmd = msg.value("command", "");
    const json& pl = msg.value("payload", json::object());
    if (cmd=="offer"||cmd=="answer") {
        Description d(pl["sdp"], cmd);
        cli.pc->setRemoteDescription(d);
        if (cmd=="offer") cli.pc->createAnswer();
    }
    else if(cmd=="ice") {
        cli.pc->addRemoteCandidate(Candidate(pl["candidate"]));
    }
}

void VoiceSfuManager::removeConnection(crow::websocket::connection* c) {
    std::lock_guard<std::mutex> lk(mtx_);
    std::cout << "[SFU] removing connection: " << c << "\n";
    // Удаляем клиента из всех комнат, где он был
    for (auto it = rooms_.begin(); it != rooms_.end();) {
        it->second.clients.erase(c);
        if (it->second.clients.empty()) {
            it = rooms_.erase(it);
        } else {
            ++it;
        }
    }
}
