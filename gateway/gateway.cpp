#include "gateway.h"
#include "commanddispatcher.h"

void GatewayServer::run(uint16_t port) {
    crow::SimpleApp app;

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([](crow::websocket::connection& conn) {
            std::cout << "WebSocket connected\n";
        })
        .onmessage([](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            std::string response = CommandDispatcher::handle(data);
            conn.send_text(response);
        })
        .onclose([](crow::websocket::connection& conn, const std::string& reason, uint16_t code) {
            std::cout << "WebSocket closed: " << reason << " (code: " << code << ")" << std::endl;
        });

    app.port(port).multithreaded().run();
}
