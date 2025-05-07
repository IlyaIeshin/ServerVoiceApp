/* helper.h */
#ifndef HELPER_GATEWAY_H
#define HELPER_GATEWAY_H

#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <crow/websocket.h>

class WsSubs {
public:
    static WsSubs& instance()
    {
        static WsSubs self;
        return self;
    }

    /** Подписать соединение на канал */
    void add(const std::string& channelId, crow::websocket::connection* c)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        subs_[channelId].insert(c);
    }

    /** Отписать соединение от канала */
    void remove(const std::string& channelId, crow::websocket::connection* c)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = subs_.find(channelId);
        if (it != subs_.end())
            it->second.erase(c);
    }

    /** Разослать текст всем подписчикам канала */
    void fanout(const std::string& channelId, const std::string& text)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto it = subs_.find(channelId);
        if (it == subs_.end()) return;

        for (auto* conn : it->second)
            if (conn)
                conn->send_text(text);
    }

    /** Удалить соединение из всех каналов (вызывается в onclose) */
    void eraseConnFromAll(crow::websocket::connection* c)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (auto& [chan, set] : subs_)
            set.erase(c);
    }

private:
    WsSubs() = default;

    std::mutex mtx_;
    std::unordered_map<std::string,
                       std::unordered_set<crow::websocket::connection*>> subs_;
};

#endif  // HELPER_GATEWAY_H
