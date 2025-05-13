#include "gateway.h"
#include "helper_gateway.h"
#include "commanddispatcher.h"

#include <crow/json.h>
#include <iostream>
#include <filesystem>

static const std::filesystem::path AVATARS_DIR =
    std::filesystem::current_path() / "media" / "avatars";

void GatewayServer::run(std::uint16_t port)
{
    crow::SimpleApp app;

    if (!std::filesystem::exists(AVATARS_DIR)) {
        std::filesystem::create_directories(AVATARS_DIR);
        CROW_LOG_INFO << "Создана папка для аватарок: " << AVATARS_DIR;
    } else {
        CROW_LOG_INFO << "Папка для аватарок уже существует: " << AVATARS_DIR;
    }

    CROW_ROUTE(app, "/upload_avatar").methods("POST"_method)
        ([this](const crow::request& req){

            try
            {
                crow::multipart::message multipart(req);

                auto it = multipart.part_map.find("avatar");
                if (it == multipart.part_map.end())
                    return crow::response(400, "No avatar uploaded");

                const auto& file_part = it->second;
                // /var/voiceServerMedia/avatars/

                std::string uuid = generate_uuid();
                std::string filename = uuid + ".png";

                auto fullpath = AVATARS_DIR / filename;
                std::ofstream out(fullpath, std::ios::binary);
                out.write(file_part.body.c_str(), file_part.body.size());
                out.close();

                crow::json::wvalue res;
                res["status"] = "ok";
                res["path"] = filename;

                return crow::response{res};
            }
            catch (const std::exception& e)
            {
                return crow::response(400, std::string("Failed to parse multipart data: ") + e.what());
            }
        });

    CROW_ROUTE(app, "/avatars/<string>")
    ([](const crow::request&, std::string filename) {
        auto fullpath = AVATARS_DIR / filename;
        if (!std::filesystem::exists(fullpath))
            return crow::response(404);

        std::ifstream file(fullpath, std::ios::binary);
        std::ostringstream ss;
        ss << file.rdbuf();

        crow::response res;
        res.code = 200;

        if (filename.ends_with(".png"))
            res.set_header("Content-Type", "image/png");
        else if (filename.ends_with(".jpg") || filename.ends_with(".jpeg"))
            res.set_header("Content-Type", "image/jpeg");
        else
            res.set_header("Content-Type", "application/octet-stream");

        res.body = ss.str();
        return res;
    });

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([](crow::websocket::connection& conn) {
            std::cout << "[Gateway] WS open from "
                      << conn.get_remote_ip() << '\n';
        })
        .onclose([](crow::websocket::connection& conn,
                    const std::string& reason,
                    std::uint16_t code) {
            std::cout << "[Gateway] WS close (" << code << "): "
                      << reason << '\n';
            {
                std::unique_lock lk(voiceMtx);

                if (auto cv = connVoiceMap.find(&conn); cv != connVoiceMap.end()) {
                    const auto& chId  = cv->second.first;
                    const auto& userId= cv->second.second;

                    if (auto it = voiceMembers.find(chId); it != voiceMembers.end()) {
                        it->second.erase(userId);
                        if (it->second.empty()) voiceMembers.erase(it);
                    }
                    connVoiceMap.erase(cv);
                }
            }

            WsSubs::instance().unsubscribeAll(&conn);
        })
        .onmessage([](crow::websocket::connection& conn,
                      const std::string& data, bool) {

            try {
                const auto req = nlohmann::json::parse(data);
                if (req.contains("cmd")) {
                    const std::string cmd = req.at("cmd");
                    if (cmd == "sub")
                        for (auto& t : req["topics"]) WsSubs::instance().subscribe(t, &conn);
                    else if (cmd == "unsub")
                        for (auto& t : req["topics"]) WsSubs::instance().unsubscribe(t, &conn);
                    nlohmann::json ok = {{"type","response"},{"cmd",cmd},{"status","ok"}};
                    conn.send_text(ok.dump());
                    return;
                }
                auto reply = CommandDispatcher::handle(conn, req);

                if (!reply.is_null() && !reply.empty())
                    conn.send_text(reply.dump());
            }
            catch (const std::exception& e) {
                nlohmann::json err = {
                    {"type","error"}, {"message", e.what()}
                };
                conn.send_text(err.dump());
            }
        });

    std::cout << "[Gateway] listening on " << port << '\n';
    app.port(port).multithreaded().run();
}

std::string GatewayServer::generate_uuid()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::string uuid = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    for (auto& ch : uuid) {
        if (ch == 'x')
            ch = "0123456789abcdef"[dis(gen)];
        else if (ch == 'y')
            ch = "89ab"[dis2(gen)];
    }
    return uuid;
}
