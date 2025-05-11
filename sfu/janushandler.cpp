#include "janushandler.h"
#include <iostream>
#include <random>
#include <sstream>

using json = nlohmann::json;

JanusHandler& JanusHandler::instance() {
    static JanusHandler inst;
    return inst;
}

JanusHandler::JanusHandler() = default;
JanusHandler::~JanusHandler() = default;

bool JanusHandler::httpPost(const std::string& path,
                            const json& req,
                            json& resp) {
    std::cout << "[JanusHandler] HTTP POST to: " << path
              << ", request: " << req.dump() << std::endl;

    httplib::Client cli("127.0.0.1", 8088);
    auto res = cli.Post(path.c_str(), req.dump(), "application/json");

    if (!res || res->status != 200) {
        std::cerr << "[JanusHandler] HTTP POST error: status="
                  << (res ? res->status : 0) << std::endl;
        return false;
    }

    try {
        resp = json::parse(res->body);
        std::cout << "[JanusHandler] HTTP POST response: " << res->body << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[JanusHandler] JSON parse error: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool JanusHandler::httpGet(const std::string& path,
                           json& resp) {
    httplib::Client cli("127.0.0.1", 8088);
    auto res = cli.Get(path.c_str());
    if (!res || res->status != 200) {
        return false;
    }
    try {
        resp = json::parse(res->body);
    } catch (...) {
        return false;
    }
    return true;
}

std::string JanusHandler::genTransaction() {
    static std::mt19937_64 rng{std::random_device{}()};
    uint64_t v = rng();
    std::ostringstream oss;
    oss << std::hex << v;
    return oss.str();
}

int64_t JanusHandler::getRoomId(const std::string& channelId) {
    auto& sess = sessions_[channelId];
    if (sess.roomId) return sess.roomId;
    sess.roomId = static_cast<int64_t>(std::hash<std::string>()(channelId));
    std::cout << "[JanusHandler] Mapped channel '" << channelId
              << "' to roomId=" << sess.roomId << std::endl;
    return sess.roomId;
}

int64_t JanusHandler::createSession() {
    json req = { {"janus", "create"}, {"transaction", genTransaction()} };
    json res;
    if (httpPost("/janus", req, res)) {
        int64_t id = res["data"]["id"].get<int64_t>();
        std::cout << "[JanusHandler] Created session: " << id << std::endl;
        return id;
    }
    return 0;
}

int64_t JanusHandler::attachPlugin(int64_t sessionId) {
    json req = { {"janus", "attach"},
                {"plugin", "janus.plugin.videoroom"},
                {"transaction", genTransaction()} };
    std::string path = "/janus/" + std::to_string(sessionId);
    json res;
    if (httpPost(path, req, res)) {
        int64_t id = res["data"]["id"].get<int64_t>();
        std::cout << "[JanusHandler] Attached plugin, handleId=" << id << std::endl;
        return id;
    }
    return 0;
}

void JanusHandler::startEventPolling(const std::string& channelId,
                                     crow::websocket::connection* conn) {
    std::thread([this, channelId, conn]() {
        auto& sess = sessions_[channelId];
        sess.rid = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());

        while (true) {
            std::string path = "/janus/" + std::to_string(sess.sessionId)
            + "?rid=" + std::to_string(sess.rid++)
                + "&maxev=10";
            json ev;
            if (!httpGet(path, ev)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            std::vector<json> events;
            if (ev.is_array()) events = ev;
            else if (ev.is_object()) events.push_back(ev);

            for (auto& e : events) {
                std::cout << "[JanusHandler] Event from Janus: " << e.dump() << std::endl;
                if (e.contains("candidate")) {
                    auto c = e["candidate"];
                    crow::json::wvalue msg;
                    msg["type"]    = "voice-signal";
                    msg["command"] = "ice";
                    msg["payload"]["channel_id"]    = channelId;
                    msg["payload"]["candidate"]     = c["candidate"].get<std::string>();
                    msg["payload"]["sdpMid"]        = c["sdpMid"].get<std::string>();
                    msg["payload"]["sdpMLineIndex"] = c["sdpMLineIndex"].get<int>();
                    conn->send_text(msg.dump());
                }
                if (e.contains("jsep")) {
                    auto j = e["jsep"];
                    crow::json::wvalue msg;
                    msg["type"]    = "voice-signal";
                    msg["command"] = j["type"].get<std::string>();
                    msg["payload"]["channel_id"] = channelId;
                    msg["payload"]["sdp"]        = j["sdp"].get<std::string>();
                    conn->send_text(msg.dump());
                }
            }
        }
    }).detach();
}

void JanusHandler::handleOffer(const std::string& channelId,
                               crow::websocket::connection* conn,
                               const json& payload) {
    std::cout << "[JanusHandler] handleOffer for channel '" << channelId
              << "', payload: " << payload.dump() << std::endl;

    auto& sess = sessions_[channelId];
    if (!sess.sessionId) {
        sess.sessionId = createSession();
        sess.handleId  = attachPlugin(sess.sessionId);
        getRoomId(channelId);
    }

    std::string root = "/janus/" + std::to_string(sess.sessionId)
                       + "/" + std::to_string(sess.handleId);
    json resp;

    if (!sess.joined) {
        if (!sess.roomCreated) {
            json b = {{"request","create"},
                      {"room", sess.roomId},
                      {"publishers", 6}};
            std::cout << "[JanusHandler] Creating room: " << sess.roomId << std::endl;
            httpPost(root, {{"janus","message"}, {"body", b}, {"transaction", genTransaction()}}, resp);
            sess.roomCreated = true;
        }

        json body = {{"request","join"},
                     {"ptype","publisher"},
                     {"room", sess.roomId},
                     {"display", channelId}};
        json jsep = {{"type","offer"},
                     {"sdp", payload["sdp"]}};
        json req = {{"janus","message"}, {"body", body}, {"jsep", jsep}, {"transaction", genTransaction()}};
        std::cout << "[JanusHandler] POST join+offer to Janus: " << req.dump() << std::endl;

        if (httpPost(root, req, resp) && resp.contains("jsep")) {
            auto j = resp["jsep"];
            crow::json::wvalue msg;
            msg["type"]    = "voice-signal";
            msg["command"] = "answer";
            msg["payload"]["channel_id"] = channelId;
            msg["payload"]["sdp"]        = j["sdp"].get<std::string>();
            std::cout << "[JanusHandler] Sending answer to client: " << msg.dump() << std::endl;
            conn->send_text(msg.dump());
            startEventPolling(channelId, conn);
        }
        sess.joined = true;
    } else {
        json cfg  = {{"request","configure"}, {"audio", true}, {"video", false}};
        json jsep = {{"type","offer"}, {"sdp", payload["sdp"]}};
        json cfgReq = {{"janus","message"}, {"body", cfg}, {"jsep", jsep}, {"transaction", genTransaction()}};
        std::cout << "[JanusHandler] POST configure to Janus: " << cfgReq.dump() << std::endl;
        httpPost(root, cfgReq, resp);
    }
}

void JanusHandler::handleIce(const std::string& channelId,
                             crow::websocket::connection*,
                             const json& payload) {
    std::cout << "[JanusHandler] handleIce for channel '" << channelId
              << "', payload: " << payload.dump() << std::endl;

    auto it = sessions_.find(channelId);
    if (it == sessions_.end()) return;
    auto& sess = it->second;
    std::string root = "/janus/" + std::to_string(sess.sessionId)
                       + "/" + std::to_string(sess.handleId);
    json resp;

    // Если получен сигнал об окончании трейлинга ICE
    if (payload.contains("candidate")
        && payload["candidate"].is_object()
        && payload["candidate"].contains("completed")
        && payload["candidate"]["completed"].get<bool>() == true) {
        json trickleReq = {
            {"janus", "trickle"},
            {"candidate", {{"completed", true}}},
            {"transaction", genTransaction()}
        };
        std::cout << "[JanusHandler] POST trickle (completed) to Janus: "
                  << trickleReq.dump() << std::endl;
        httpPost(root, trickleReq, resp);
        return;
    }

    // Обычный ICE-кандидат
    json cand = {
        {"candidate", payload["candidate"]},
        {"sdpMid",    payload["sdpMid"]},
        {"sdpMLineIndex", payload["sdpMLineIndex"]}
    };
    json trickleReq = {
        {"janus", "trickle"},
        {"candidate", cand},
        {"transaction", genTransaction()}
    };
    std::cout << "[JanusHandler] POST trickle to Janus: " << trickleReq.dump() << std::endl;
    httpPost(root, trickleReq, resp);
}

void JanusHandler::handleSubscribe(const std::string& channelId,
                                   crow::websocket::connection* conn,
                                   int feedId) {
    std::cout << "[JanusHandler] handleSubscribe for channel '" << channelId
              << "', feedId=" << feedId << std::endl;

    auto it = sessions_.find(channelId);
    if (it == sessions_.end()) return;
    auto& sess = it->second;
    int64_t subHandle = attachPlugin(sess.sessionId);
    std::string root = "/janus/" + std::to_string(sess.sessionId)
                       + "/" + std::to_string(subHandle);
    json resp;
    json body = {{"request","join"}, {"ptype","subscriber"}, {"room", sess.roomId}, {"feed", feedId}};
    json subReq = {{"janus","message"}, {"body", body}, {"transaction", genTransaction()}};
    std::cout << "[JanusHandler] POST subscribe to Janus: " << subReq.dump() << std::endl;

    if (httpPost(root, subReq, resp)) {
        auto j = resp["jsep"];
        crow::json::wvalue msg;
        msg["type"]    = "voice-signal";
        msg["command"] = "offer";
        msg["payload"]["channel_id"] = channelId;
        msg["payload"]["sdp"]        = j["sdp"].get<std::string>();
        std::cout << "[JanusHandler] Sending subscriber offer to client: " << msg.dump() << std::endl;
        conn->send_text(msg.dump());
        startEventPolling(channelId, conn);
    }
}
