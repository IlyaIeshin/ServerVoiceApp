#include "voicesfumanager.h"
#include <iostream>

VoiceSfuManager& VoiceSfuManager::instance() {
    static VoiceSfuManager inst;
    return inst;
}

VoiceSfuManager::VoiceSfuManager()
    : sock_(ioc_, udp::endpoint(udp::v4(), 40000))
{
    std::cout << "[SFU] Constructor: UDP listening on port 40000" << std::endl;
    doReceive();
    thread_ = std::thread([this]() {
        std::cout << "[SFU] io_context running" << std::endl;
        ioc_.run();
    });
    thread_.detach();
}

VoiceSfuManager::~VoiceSfuManager() {
    std::cout << "[SFU] Destructor: stopping io_context" << std::endl;
    ioc_.stop();
}

void VoiceSfuManager::doReceive() {
    std::cout << "[SFU] Waiting for incoming UDP packets..." << std::endl;
    sock_.async_receive_from(
        boost::asio::buffer(buf_), sender_,
        [this](boost::system::error_code ec, std::size_t len) {
            if (!ec) handlePacket(len);
            else std::cout << "[SFU] Receive error:" << ec.message() << std::endl;
            doReceive();
        }
        );
}

void VoiceSfuManager::handlePacket(std::size_t len) {
    std::cout << "[SFU] Packet received: length=" << len
              << ", from " << sender_.address().to_string()
              << ":" << sender_.port() << std::endl;
    if (len == 0) return;
    if (buf_[0] == '{') {
        std::string raw(buf_.data(), len);
        std::cout << "[SFU] JSON command received: " << raw << std::endl;
        try {
            json j = json::parse(raw);
            std::string cmd = j.at("command").get<std::string>();
            std::string channel = j.at("channel").get<std::string>();
            std::string peer = j.value("peer", "");

            if (cmd == "join") {
                channels_[channel].peers.insert(sender_);
                peerToChannel_[sender_] = channel;
                std::cout << "[SFU] Peer '" << peer << "' joined channel '"
                          << channel << "'" << std::endl;
            }
            else if (cmd == "leave") {
                auto it = channels_.find(channel);
                if (it != channels_.end()) {
                    it->second.peers.erase(sender_);
                    peerToChannel_.erase(sender_);
                    std::cout << "[SFU] Peer '" << peer << "' left channel '"
                              << channel << "'" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[SFU] JSON parse error: " << e.what() << std::endl;
        }
    } else {
        auto it = peerToChannel_.find(sender_);
        if (it != peerToChannel_.end()) {
            const std::string& channelId = it->second;
            auto cit = channels_.find(channelId);
            if (cit != channels_.end()) {
                for (const auto& ep : cit->second.peers) {
                    if (ep != sender_) {
                        sock_.send_to(boost::asio::buffer(buf_.data(), len), ep);
                        std::cout << "[SFU] Relayed packet of size " << len
                                  << " to " << ep.address().to_string()
                                  << ":" << ep.port() << std::endl;
                    }
                }
            }
        } else {
            std::cout << "[SFU] Unknown sender, not in any channel" << std::endl;
        }
    }
}

void VoiceSfuManager::handleSignal(const std::string& /*channelId*/,
                                   crow::websocket::connection* /*conn*/,
                                   const json& /*req*/) {
    std::cout << "[SFU] handleSignal called - not used in UDP SFU" << std::endl;
}
