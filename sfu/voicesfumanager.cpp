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
    if (len == 0) return;

    if (buf_[0] == '{') {
        std::string raw(buf_.data(), len);
        std::cout << "[SFU] JSON command received: " << raw << std::endl;
        try {
            json j = json::parse(raw);
            std::string cmd     = j.at("command").get<std::string>();
            std::string channel = j.at("channel").get<std::string>();
            std::string peer    = j.value("peer", "");

            if (cmd == "join") {
                channels_[channel].peers.insert(sender_);
                peerToChannel_[sender_] = channel;
                peerId_[sender_] = peer;
            }
            else if (cmd == "leave") {
                auto it = channels_.find(channel);
                if (it != channels_.end()) {
                    it->second.peers.erase(sender_);
                    peerToChannel_.erase(sender_);
                    peerId_.erase(sender_);
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[SFU] JSON parse error:" << e.what() << std::endl;
        }
    } else {
        auto it = peerToChannel_.find(sender_);
        if (it != peerToChannel_.end()) {
            const std::string& channelId = it->second;
            auto cit = channels_.find(channelId);
            if (cit != channels_.end()) {
                auto pidIter = peerId_.find(sender_);
                std::string pid = (pidIter != peerId_.end() ? pidIter->second : "");
                for (const auto& ep : cit->second.peers) {
                    if (ep != sender_) {
                        uint16_t pidLen = static_cast<uint16_t>(pid.size());
                        std::string sendBuf;
                        sendBuf.reserve(2 + pidLen + len);
                        sendBuf.push_back(static_cast<char>((pidLen >> 8) & 0xFF));
                        sendBuf.push_back(static_cast<char>( pidLen        & 0xFF));
                        sendBuf.append(pid);
                        sendBuf.append(buf_.data(), len);
                        sock_.send_to(boost::asio::buffer(sendBuf.data(), sendBuf.size()), ep);
                    }
                }
            }
        } else {
            std::cout << "[SFU] Unknown sender, not in any channel" << std::endl;
        }
    }
}
