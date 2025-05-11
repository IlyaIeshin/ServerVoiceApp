// VoiceSfuManager.cpp
#include "voicesfumanager.h"
using json = nlohmann::json;
using namespace rtc;

VoiceSfuManager& VoiceSfuManager::instance() {
    // Инициализация логгера (вызываем один раз)
    static bool loggerInitialized = false;
    if (!loggerInitialized) {
        rtc::InitLogger(rtc::LogLevel::Verbose); // или Verbose для отладки
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
    auto& room = rooms_[chId];
    auto& cli  = room.clients[c];

    // Если для данного соединения ещё нет PeerConnection – создаём
    if (!cli.pc) {
        // Конфигурация PeerConnection
        Configuration config;
        config.iceServers = {{"stun:stun.l.google.com:19302"}};
        cli.pc = std::make_shared<PeerConnection>(config);

        // Callback: локальное описание (offer/answer)
        cli.pc->onLocalDescription([=](Description desc) {
            std::string sdpType = desc.typeString(); // "offer" или "answer"
            std::cout << "[SFU] send " << sdpType << " to client\n";
            json out;
            out["type"] = "voice-signal";
            out["command"] = sdpType;
            out["payload"] = {
                {"channel_id", chId},
                {"sdp", std::string(desc)}
            };
            c->send_text(out.dump());
        });

        // Callback: локальный ICE-кандидат
        cli.pc->onLocalCandidate([=](Candidate cand) {
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
            std::cout << "[SFU] onTrack: incoming\n";
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
                                 std::cout << "[SERVER] Received audio frame (ts="
                                           << fi.timestamp << ", size=" << frame.size() << " bytes)\n";

                                 // Пересылаем кадр каждому другому участнику комнаты
                                 for (auto& [otherConn, otherCli] : room.clients) {
                                     if (otherConn == c || !otherCli.pc) continue; // себе не шлём

                                     // Для каждого получателя создаём или получаем свой downTrack
                                     auto& downTrack = otherCli.downTracks[inTrack.get()];
                                     if (!downTrack) {
                                         // Создаём аудио-трек для передачи этому клиенту
                                         Description::Audio outDesc("forward-audio", Description::Direction::SendOnly);
                                         outDesc.addOpusCodec(111);  // поддерживаем Opus, PT=111
                                         downTrack = otherCli.pc->addTrack(outDesc);
                                         downTrack->onOpen([=]() {
                                             std::cout << "[SFU] downTrack opened for a receiver\n";
                                         });
                                     }

                                     // Отправляем аудио-фрейм получателю
                                     if (downTrack->isOpen()) {
                                         downTrack->sendFrame(frame, fi);
                                     } else {
                                         std::cout << "[SFU] warning: try sendFrame while track not open\n";
                                         downTrack->sendFrame(frame, fi);
                                     }
                                 }
                             });
        }); // end onTrack
    } // if !cli.pc

    // Обработка входящего сигналинга (SDP и ICE) от клиента
    std::string cmd = msg.value("command", "");
    const json& pl = msg.value("payload", json::object());
    if (cmd == "offer" || cmd == "answer") {
        // Удалённый SDP
        Description desc(pl.at("sdp").get<std::string>(), cmd);
        cli.pc->setRemoteDescription(desc);
        if (cmd == "offer") {
            // Клиент прислал Offer -> формируем Answer
            cli.pc->createAnswer();
        }
    }
    else if (cmd == "ice") {
        // Удалённый ICE-кандидат
        Candidate cand(pl.at("candidate").get<std::string>());
        cli.pc->addRemoteCandidate(cand);
    }
}

void VoiceSfuManager::removeConnection(crow::websocket::connection* c) {
    std::lock_guard<std::mutex> lk(mtx_);
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
