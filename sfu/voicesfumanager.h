// voicesfumanager.h
#ifndef VOICESFU_MANAGER_H
#define VOICESFU_MANAGER_H

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <set>
#include <array>
#include <string>
#include <thread>

namespace crow { namespace websocket { struct connection; } }

using boost::asio::ip::udp;
using json = nlohmann::json;

// Хеш и сравнение для udp::endpoint
struct EndpointHash {
    std::size_t operator()(const udp::endpoint& ep) const noexcept {
        auto addr_str = ep.address().to_string();
        std::size_t h1 = std::hash<std::string>()(addr_str);
        std::size_t h2 = std::hash<unsigned short>()(ep.port());
        // Комбинируем хеш адреса и порта
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1<<6) + (h1>>2));
    }
};
struct EndpointEqual {
    bool operator()(const udp::endpoint& a, const udp::endpoint& b) const noexcept {
        return (a.address() == b.address() && a.port() == b.port());
    }
};
// Структура канала с набором UDP-эпандпоинтов участников
struct ChannelSfu {
    std::set<udp::endpoint> peers;
};

class VoiceSfuManager {
public:
    // Singleton
    static VoiceSfuManager& instance();

    // Обработка сигнала (не используется в данном примере UDP-SFU)
    void handleSignal(const std::string& channelId,
                      crow::websocket::connection* conn,
                      const json& req);

private:
    VoiceSfuManager();
    ~VoiceSfuManager();
    VoiceSfuManager(const VoiceSfuManager&) = delete;
    VoiceSfuManager& operator=(const VoiceSfuManager&) = delete;

    void doReceive();
    void handlePacket(std::size_t len);

    boost::asio::io_context ioc_;
    udp::socket sock_;
    udp::endpoint sender_;
    std::array<char, 4096> buf_;

    // Мэппинги: channelId -> Channel и endpoint -> channelId
    std::unordered_map<std::string, ChannelSfu> channels_;
    std::unordered_map<udp::endpoint, std::string, EndpointHash, EndpointEqual> peerToChannel_;

    std::thread thread_;
};

#endif // VOICESFU_MANAGER_H
