#include "commanddispatcher.h"
#include "jwtservice.h"
#include "helper_gateway.h"
#include <sstream>

using json     = nlohmann::json;
using JwtToken = std::string;

PostgresHandler  CommandDispatcher::psql_handler{"dbname=postgres user=ilyaieshin password=030609 host=localhost port=5432"};
CassandraHandler CommandDispatcher::cass_handler{"127.0.0.1"};


json CommandDispatcher::handle(crow::websocket::connection& conn, const json& req)
{
    json response;
    const std::string type    = req.value("type",    "");
    const std::string command = req.value("command", "");

    try {
        const auto& payload = req.value("payload", json::object());

        static UserRepository    rep_user  (psql_handler);
        static ServerRepository  rep_server(psql_handler);
        static ChannelRepository rep_channel(psql_handler, cass_handler);
        static JwtService        jwt_serv("secret_token");

        /* =============== АВТОРИЗАЦИЯ / РЕГИСТРАЦИЯ =============== */
        if (type == "command" && command == "loadToken") {
            std::string user_id = jwt_serv.verifyToken(payload.at("token"));
            Result result = rep_user.getUserData(user_id);

            json user_json = {
                {"user_id",         result.value.id},
                {"name",       result.value.username},
                {"avatar_url", result.value.avatar_url.empty() ? "" : result.value.avatar_url}
            };

            response = {
                {"type",    "response"},
                {"command", "loadToken"},
                {"status",  "ok"},
                {"data",    {{"user_json", user_json}}},
                {"error-msg", json::object()}
            };
        }
        else if (type == "command" && command == "register") {
            Result result = rep_user.registerUser(payload.at("email"),
                                                  payload.at("username"),
                                                  payload.at("password"),
                                                  payload.at("avatar_url"));

            switch (result.getResult()) {
            case Error::None: {
                JwtToken jwt = jwt_serv.generateToken(result.value.id);

                json user_json = {
                    {"user_id",         result.value.id},
                    {"username",       result.value.username},
                    {"avatar_url", result.value.avatar_url.empty() ? "" : result.value.avatar_url},
                    {"token", jwt}
                };

                response = {
                    {"type",    "response"},
                    {"command", "register"},
                    {"status",  "ok"},
                    {"data",    {{"user_json", user_json}}},
                    {"error-msg", json::object()}
                }; break; }
            case Error::EmailAlreadyUsed: {
                response = {
                    {"type",    "response"},
                    {"command", "register"},
                    {"status",  "error"},
                    {"data",    ResponseOK{{"email", payload.at("email")}}},
                    {"error-msg", "email already used"}
                }; break; }
            case Error::UsernameAlreadyUsed: {
                response = {
                    {"type",    "response"},
                    {"command", "register"},
                    {"status",  "error"},
                    {"data",    ResponseOK{{"username", payload.at("username")}}},
                    {"error-msg", "username already used"}
                }; break; }
            case Error::DbError:
            default: {
                response = {
                    {"type",    "response"},
                    {"command", "register"},
                    {"status",  "error"},
                    {"data",    ResponseOK{}},
                    {"error-msg", "unknown error database"}
                }; break; }
            }
        }
        else if (type == "command" && command == "authentication") {
            Result result = rep_user.loginUser(payload.at("email"), payload.at("password"));

            switch (result.getResult()) {
            case Error::None: {
                JwtToken jwt = jwt_serv.generateToken(result.value.id);

                json user_json = {
                    {"user_id",         result.value.id},
                    {"username",       result.value.username},
                    {"avatar_url", result.value.avatar_url.empty() ? "" : result.value.avatar_url},
                    {"token", jwt}
                };

                response = {
                    {"type",    "response"},
                    {"command", "authentication"},
                    {"status",  "ok"},
                    {"data",    {{"user_json", user_json}}},
                    {"error-msg", json::object()}
                }; break; }
            case Error::UserNotFound: {
                response = {
                    {"type",    "response"},
                    {"command", "authentication"},
                    {"status",  "error"},
                    {"data",    ResponseOK{{"email", payload.at("email")}}},
                    {"error-msg", "user not found"}
                }; break; }
            case Error::WrongPassword: {
                response = {
                    {"type",    "response"},
                    {"command", "authentication"},
                    {"status",  "error"},
                    {"data",    ResponseOK{}},
                    {"error-msg", "wrong password"}
                }; break; }
            case Error::DbError:
            default: {
                response = {
                    {"type",    "response"},
                    {"command", "authentication"},
                    {"status",  "error"},
                    {"data",    ResponseOK{}},
                    {"error-msg", "unknown error database"}
                }; break; }
            }
        }

        /* ======================== СЕРВЕРЫ ======================== */
        else if (command == "createServer") {
            Result result = rep_user.createServer(payload.at("name"),
                                                  payload.at("user_id"),
                                                  payload.at("password"),
                                                  payload.at("icon_url"));
            json server_json = {
                {"id",       result.value.id},
                {"name",     result.value.name},
                {"icon_url", result.value.icon_url.empty() ? "" : result.value.icon_url}
            };
            switch (result.getResult()) {
            case Error::None: {
                response = {
                    {"type",    "response"},
                    {"command", "createServer"},
                    {"status",  "ok"},
                    {"data",    {{"server_json", server_json}}},
                    {"error-msg", json::object()}
                }; break; }
            case Error::ServerAlreadyExists: {
                response = {
                    {"type",    "response"},
                    {"command", "createServer"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "server name already used"}
                }; break; }
            case Error::DbError:
            default: {
                response = {
                    {"type",    "response"},
                    {"command", "createServer"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                }; break; }
            }
        }
        else if (command == "leaveServer") {
            Result result = rep_user.leaveServer(payload.at("user_id"), payload.at("server_id"));
            switch (result.getResult()) {
            case Error::None: {
                response = {
                    {"type",    "response"},
                    {"command", "leaveServer"},
                    {"status",  "ok"},
                    {"data",    {{"server_id", result.value}}},
                    {"error-msg", json::object()}
                }; break; }
            case Error::DbError:
            default: {
                response = {
                    {"type",    "response"},
                    {"command", "leaveServer"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                }; break; }
            }
        }

        /* ======================== КАНАЛЫ ======================== */
        else if (command == "createChannel") {
            Result result = rep_server.createChannel(payload.at("name"),
                                                     payload.at("password"),
                                                     payload.at("type_channel"),
                                                     payload.at("server_id"),
                                                     payload.at("owner_id"));
            json channel_json = {
                {"id",           result.value.id},
                {"name",         result.value.name},
                {"type_channel", result.value.type},
                {"server_id",    payload.at("server_id")}
            };
            switch (result.getResult()) {
            case Error::None: {
                response = {
                    {"type",    "response"},
                    {"command", "createChannel"},
                    {"status",  "ok"},
                    {"data",    {{"channel_json", channel_json}}},
                    {"error-msg", json::object()}
                }; break; }
            case Error::ChannelAlreadyExists: {
                response = {
                    {"type",    "response"},
                    {"command", "createChannel"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "channel name already used"}
                }; break; }
            case Error::DbError:
            default: {
                response = {
                    {"type",    "response"},
                    {"command", "createChannel"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                }; break; }
            }
        }
        else if (command == "getAllChannels") {
            std::vector<std::string> serverIds = split(payload.at("server_ids"), ',');
            Result result = rep_server.getAllChannels(serverIds);
            if (result.getResult() == Error::None) {
                json arr = json::array();
                for (const auto& row : result.value) {
                    arr.push_back({{"id", row.id},{"name", row.name},{"type_channel", row.type},{"server_id", row.server_id}});
                }
                response = {
                    {"type",    "response"},
                    {"command", "getAllChannels"},
                    {"status",  "ok"},
                    {"data",    {{"channels", arr}}},
                    {"error-msg", json::object()}
                };
            } else {
                response = {
                    {"type",    "response"},
                    {"command", "getAllChannels"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                };
            }
        }

        /* =============== ПОДПИСКИ НА ТЕКСТОВЫЕ КАНАЛЫ =============== */
        else if (command == "subscribeTextChannel") {
            WsSubs::instance().add(payload.at("channel_id"), &conn);
            response = {
                {"type",    "response"},
                {"command", "subscribeTextChannel"},
                {"status",  "ok"},
                {"data",    json::object()}
            };
        }
        else if (command == "unsubscribeTextChannel") {
            WsSubs::instance().remove(payload.at("channel_id"), &conn);
            response = {
                {"type",    "response"},
                {"command", "unsubscribeTextChannel"},
                {"status",  "ok"},
                {"data",    json::object()}
            };
        }
        else if (command == "sendMessageToTextChannel") {
            Result result = rep_channel.addMessage(payload.at("user_id"),
                                                   payload.at("text_message"),
                                                   payload.at("channel_id"));
            if (result.getResult() == Error::None) {
                json message_json = {
                    {"id",           result.value.message_id},
                    {"sender_id",    result.value.sender_id},
                    {"channel_id",   result.value.channel_id},
                    {"text_message", result.value.text},
                    {"created_at",   result.value.created_at}
                };
                json event = {
                    {"type",    "event"},
                    {"command", "newMessage"},
                    {"data",    {{"message_json", message_json}}}
                };
                WsSubs::instance().fanout(payload.at("channel_id"), event.dump());

                response = {
                    {"type",    "response"},
                    {"command", "sendMessageToTextChannel"},
                    {"status",  "ok"},
                    {"data",    {{"message_json", message_json}}}
                };
            } else {
                response = {
                    {"type",    "response"},
                    {"command", "sendMessageToTextChannel"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                };
            }
        }
        else if (command == "getMessagesFromChannel" || command == "getMessagesBefore")
        {
            std::string chanId  = payload.at("channel_id");
            std::string tokenIn = payload.value("pageState", "");

            auto r = rep_channel.getMessagesPage(chanId, tokenIn);

            if (r.getResult() == Error::None) {
                json arr = json::array();
                for (auto& m : r.value.msgs) {
                    arr.push_back({
                        {"message_id", m.message_id},
                        {"channel_id", m.channel_id},
                        {"sender_id",  m.sender_id},
                        {"created_at", m.created_at},
                        {"text",       m.text}
                    });
                }
                response = {
                    {"type","response"}, {"command",command},
                    {"status","ok"},
                    {"data",{
                                 {"messages",  arr},
                                 {"pageState", r.value.nextPageState}
                             }}
                };
            } else {
                response = { {"type","response"}, {"command",command},
                            {"status","error"},
                            {"data",json::object()},
                            {"error-msg","db error"} };
            }
        }

        /* =============== ПОДПИСКИ НА ТЕКСТОВЫЕ КАНАЛЫ =============== */
        else if (command == "subscribeVoiceChannel") {

        }
        else if (command == "unsubscribeVoiceChannel") {

        }

        /* ======================== СПИСОК СЕРВЕРОВ ======================== */
        else if (command == "getServers") {
            Result result = rep_user.getUserServers(payload.at("user_id"));
            if (result.getResult() == Error::None) {
                json arr = json::array();
                for (const auto& row : result.value) {
                    arr.push_back({{"id", row.id}, {"name", row.name}, {"icon_url", row.icon_url.empty() ? "" : row.icon_url}});
                }
                response = {
                    {"type",    "response"},
                    {"command", "getServers"},
                    {"status",  "ok"},
                    {"data",    {{"servers", arr}}}
                };
            } else {
                response = {
                    {"type",    "response"},
                    {"command", "getServers"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                };
            }
        }

        /* ======================== ДРУЗЬЯ ======================== */
        else if (command == "getFriends") {
            Result result = rep_user.getFriends(payload.at("user_id"));
            if (result.getResult() == Error::None) {
                json arr = json::array();
                for (const auto& row : result.value) {
                    arr.push_back({{"id", row.id}, {"username", row.username}, {"avatar_url", row.avatar_url.empty() ? "" : row.avatar_url}, {"status", row.status.empty() ? "offline" : row.status}});
                }
                response = {
                    {"type",    "response"},
                    {"command", "getFriends"},
                    {"status",  "ok"},
                    {"data",    {{"friends", arr}}}
                };
            } else {
                response = {
                    {"type",    "response"},
                    {"command", "getFriends"},
                    {"status",  "error"},
                    {"data",    json::object()},
                    {"error-msg", "unknown error database"}
                };
            }
        }

        /* ==================== НЕИЗВЕСТНАЯ КОМАНДА ==================== */
        else {
            response = {
                {"type", "response"},
                {"command", command},
                {"status", "error"},
                {"data", json::object()},
                {"error-msg", "unknown command"}
            };
        }
    }
    catch (const std::exception& e) {
        response = {
            {"type", "response"},
            {"command", command},
            {"status", "error"},
            {"data", json::object()},
            {"error-msg", e.what()}
        };
    }

    return response;        // Gateway сам сделает dump()
}


std::vector<std::string>
CommandDispatcher::split(const std::string& str, char delim)
{
    std::vector<std::string> out;
    std::istringstream ss(str);
    std::string tok;
    while (std::getline(ss, tok, delim))
        if (!tok.empty()) out.emplace_back(tok);
    return out;
}
