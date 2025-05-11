#pragma once

#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <crow.h>
#include <httplib/httplib.h>
#include <chrono>
#include <thread>

// JanusHandler: проксирует сигнальные сообщения от Crow к Janus WebRTC Server
class JanusHandler {
public:
    static JanusHandler& instance();

    // Обработка SDP offer от клиента
    void handleOffer(const std::string& channelId,
                     crow::websocket::connection* conn,
                     const nlohmann::json& payload);
    // Обработка ICE-кандидата от клиента
    void handleIce(const std::string& channelId,
                   crow::websocket::connection* conn,
                   const nlohmann::json& payload);
    // Обработка запроса на подписку на feed другого клиента
    void handleSubscribe(const std::string& channelId,
                         crow::websocket::connection* conn,
                         int feedId);

private:
    JanusHandler();
    ~JanusHandler();

    // HTTP POST к Janus
    bool httpPost(const std::string& path,
                  const nlohmann::json& req,
                  nlohmann::json& resp);
    // HTTP GET для long-polling событий
    bool httpGet(const std::string& path,
                 nlohmann::json& resp);
    // Генерация transaction
    std::string genTransaction();
    // Маппинг channelId -> numeric roomId
    int64_t getRoomId(const std::string& channelId);
    // Создание сессии и плагина
    int64_t createSession();
    int64_t attachPlugin(int64_t sessionId);

    // Запуск long-polling событий для session
    void startEventPolling(const std::string& channelId,
                           crow::websocket::connection* conn);

    struct Session {
        int64_t sessionId   = 0;
        int64_t handleId    = 0;
        int64_t roomId      = 0;
        bool    roomCreated = false;
        bool    joined      = false;
        uint64_t rid        = 0;
    };
    std::unordered_map<std::string, Session> sessions_;
};
