// VoiceSfuManager.h
#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include <crow/websocket.h>
#include <rtc/rtc.hpp>
#include <nlohmann/json.hpp>

class VoiceSfuManager {
public:
    static VoiceSfuManager& instance();

    void handleSignal(const std::string& chId,
                      crow::websocket::connection* wsConn,
                      const nlohmann::json& msg);

    void removeConnection(crow::websocket::connection* wsConn);

private:
    struct Client {
        crow::websocket::connection*               conn;
        std::shared_ptr<rtc::PeerConnection>       pc;
        std::shared_ptr<rtc::Track>                uplink;    // входящий трек от клиента
        std::unordered_map<const rtc::Track*, std::shared_ptr<rtc::Track>> downTracks;
    };
    struct Room {
        std::unordered_map<crow::websocket::connection*, Client> clients;
    };

    std::mutex mtx_;
    std::unordered_map<std::string, Room> rooms_;
};
