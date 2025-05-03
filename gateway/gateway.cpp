/* gateway.cpp */
#include "gateway.h"
#include "helper_gateway.h"              //  WsSubs
#include "commanddispatcher.h"

#include <crow/json.h>
#include <iostream>

void GatewayServer::run(std::uint16_t port)
{
    crow::SimpleApp app;

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([](crow::websocket::connection& conn) {
            std::cout << "[Gateway] WS open from "
                      << conn.get_remote_ip() << '\n';
        })
        .onclose([](crow::websocket::connection& conn,
                    const std::string& reason,
                    std::uint16_t code) {
            std::cout << "[Gateway] WS close (" << code
                      << "): " << reason << '\n';
            WsSubs::instance().eraseConnFromAll(&conn);
        })
        .onmessage([](crow::websocket::connection& conn,
                      const std::string& data, bool) {

            try {
                const auto req = nlohmann::json::parse(data);
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
