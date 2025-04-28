#include "commanddispatcher.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <db/request_repositories.h>
#include "jwtservice.h"
#include <any>

using json = nlohmann::json;
using JwtToken = std::string;

PostgresHandler CommandDispatcher::psql_handler{"dbname=postgres user=ilyaieshin password=030609 host=localhost port=5432"};
CassandraHandler CommandDispatcher::cass_handler{};



std::string CommandDispatcher::handle(const std::string& input) {
    json response;
    auto j = json::parse(input);
    std::string type = j["type"];
    std::string command = j["command"];
    try {

        std::map<std::string, std::string> payload = j["payload"];

        static UserRepository rep_user(psql_handler);
        static ServerRepository rep_server(psql_handler);
        static JwtService jwt_serv("secret_token");
        if(type == "command") {
            if(command == "loadToken") {
                std::string user_id = jwt_serv.verifyToken(payload["token"]);
                response["type"] = "response";
                response["command"] = "loadToken";
                response["status"] = "ok";
                response["data"] = ResponseOK{{"user_id", user_id}};
                response["error-msg"] = {};
            }
            else if(command == "register") {
                Result result = rep_user.registerUser(payload["email"], payload["username"], payload["password"]);
                switch (result.getResult()) {
                case Error::None:{
                    JwtToken jwt = jwt_serv.generateToken(result.value);
                    response["type"] = "response";
                    response["command"] = "register";
                    response["status"] = "ok";
                    response["data"] = ResponseOK{
                        {"token", jwt},
                    };
                    response["error-msg"] = {};

                    break;
                }
                case Error::EmailAlreadyUsed:{
                    response["type"] = "response";
                    response["command"] = "register";
                    response["status"] = "error";
                    response["data"] = ResponseOK{{"email", payload["email"]}};
                    response["error-msg"] = "email already used";
                    break;
                }
                case Error::UsernameAlreadyUsed:{
                    response["type"] = "response";
                    response["command"] = "register";
                    response["status"] = "error";
                    response["data"] = ResponseOK{{"username", payload["username"]}};
                    response["error-msg"] = "username already used";
                    break;
                }
                case Error::DbError:{
                    response["type"] = "response";
                    response["command"] = "register";
                    response["status"] = "error";
                    response["data"] = ResponseOK{};
                    response["error-msg"] = "unknown error database";
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "authentication") {
                Result result = rep_user.loginUser(payload["email"], payload["password"]);

                switch (result.getResult()) {
                case Error::None:{
                    JwtToken jwt = jwt_serv.generateToken(result.value);
                    response["type"] = "response";
                    response["command"] = "authentication";
                    response["status"] = "ok";
                    response["data"] = ResponseOK{
                        {"token", jwt},
                    };
                    response["error-msg"] = {};
                    break;
                }
                case Error::UserNotFound:{
                    response["type"] = "response";
                    response["command"] = "authentication";
                    response["status"] = "error";
                    response["data"] = ResponseOK{{"email", payload["email"]}};
                    response["error-msg"] = "user not found";
                    break;
                }
                case Error::WrongPassword:{
                    response["type"] = "response";
                    response["command"] = "authentication";
                    response["status"] = "error";
                    response["data"] = ResponseOK{};
                    response["error-msg"] = "wrong password";
                    break;
                }
                case Error::DbError:{
                    response["type"] = "response";
                    response["command"] = "authentication";
                    response["status"] = "error";
                    response["data"] = ResponseOK{};
                    response["error-msg"] = "unknown error database";
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "createServer") {
                Result result = rep_user.createServer(payload["name"], payload["user_id"], payload["password"], payload["icon_url"]);

                json server_json = {
                    {"id", result.value.id},
                    {"name", result.value.name},
                    {"icon_url", result.value.icon_url.empty() ? "" : result.value.icon_url}
                };

                switch (result.getResult()) {
                case Error::None:{
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "ok";
                    response["data"]["server_json"] = server_json;
                    response["error-msg"] = {};
                    break;
                }
                case Error::ServerAlreadyExists: {
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "error";
                    response["data"] = {};
                    response["error-msg"] = "server name already used";
                    break;
                }
                case Error::DbError:{
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "error";
                    response["data"] = {};
                    response["error-msg"] = "unknown error database";
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "leaveServer") {
                Result result = rep_user.leaveServer(payload["user_id"], payload["server_id"]);
                switch (result.getResult()) {
                case Error::None:{
                    response["type"] = "response";
                    response["command"] = "leaveServer";
                    response["status"] = "ok";
                    response["data"]["server_id"] = result.value;
                    response["error-msg"] = {};
                    break;
                }
                case Error::DbError:{
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "error";
                    response["data"] = {};
                    response["error-msg"] = "unknown error database";
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "joinToServer") {

            }
            else if(command == "createChannel") {
                Result result = rep_server.createChannel(payload["name"], payload["password"], payload["type_channel"], payload["server_id"], payload["owner_id"]);

                json channel_json = {
                    {"id", result.value.id},
                    {"name", result.value.name},
                    {"type_channel", result.value.type},
                    {"server_id", payload["server_id"]}
                };

                switch (result.getResult()) {
                case Error::None: {
                    response["type"] = "response";
                    response["command"] = "createChannel";
                    response["status"] = "ok";
                    response["data"]["channel_json"] = channel_json;
                    response["error-msg"] = {};
                    break;
                }
                case Error::ChannelAlreadyExists: {
                    response["type"] = "response";
                    response["command"] = "createChannel";
                    response["status"] = "error";
                    response["data"] = {};
                    response["error-msg"] = "channel name already used";
                    break;
                }
                case Error::DbError:{
                    response["type"] = "response";
                    response["command"] = "createChannel";
                    response["status"] = "error";
                    response["data"] = {};
                    response["error-msg"] = "unknown error database";
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "getAllChannels") {
                std::string ids = payload["server_ids"];
                std::vector<std::string> serverIds = split(ids, ',');
                Result result = rep_server.getAllChannels(serverIds);
                switch(result.getResult()) {
                case Error::None: {
                    json channels = json::array();
                    for(const auto& row : result.value) {
                        json channel_json = {
                            {"id", row.id},
                            {"name", row.name},
                            {"type_channel", row.type},
                            {"server_id", row.server_id}
                        };
                        channels.push_back(channel_json);
                    }
                    response["type"] = "response";
                    response["command"] = "getAllChannels";
                    response["status"] = "ok";
                    response["data"] = {{"channels", channels}};
                    response["error-msg"] = {};
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "joinToChannel") {

            }
            else if(command == "sendMessageToTextChannel") {
                response["type"] = "response";
                response["command"] = "sendMessageToTextChannel";
                response["status"] = "ok";
                response["data"] = {};
                response["error-msg"] = {};
            }
            else if(command == "getServers") {
                Result result = rep_user.getUserServers(payload["user_id"]);
                switch(result.getResult()) {
                case Error::None: {
                    json servers = json::array();
                    for(const auto& row : result.value) {
                        json server_json = {
                            {"id", row.id},
                            {"name", row.name},
                            {"icon_url", row.icon_url.empty() ? "" : row.icon_url}
                        };
                        servers.push_back(server_json);
                    }

                    response["type"] = "response";
                    response["command"] = "getServers";
                    response["status"] = "ok";
                    response["data"] = {{"servers", servers}};
                    response["error-msg"] = {};
                    break;
                }
                default:
                    break;
                }
            }
            else if(command == "getFriends") {
                Result result = rep_user.getFriends(payload["user_id"]);
                switch(result.getResult()) {
                case Error::None: {
                    json friends = json::array();

                    for(const auto& row : result.value) {
                        json friend_json = {
                            {"id", row.id},
                            {"username", row.username},
                            {"avatar_url", row.avatar_url.empty() ? "" : row.avatar_url},
                            {"status", row.status.empty() ? "offline" : row.status}
                        };
                        friends.push_back(friend_json);
                    }

                    response["type"] = "response";
                    response["command"] = "getFriends";
                    response["status"] = "ok";
                    response["data"] = {{"friends", friends}};
                    response["error-msg"] = {};
                    break;
                }
                default:
                    break;
                }
            }
        }

        return response.dump();
    } catch (...) {
        response["type"] = "response";
        response["command"] = command;
        response["status"] = "error";
        response["data"] = {};
        response["error-msg"] = "invalid request";
        return response.dump();
    }
}

std::vector<std::string> CommandDispatcher::split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream stream(str);
    std::string token;
    while (std::getline(stream, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}
