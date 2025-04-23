#include "commanddispatcher.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <db/request_repositories.h>
#include "jwtservice.h"

using json = nlohmann::json;
using JwtToken = std::string;

PostgresHandler CommandDispatcher::psql_handler{"dbname=postgres user=ilyaieshin password=030609 host=localhost port=5432"};
CassandraHandler CommandDispatcher::cass_handler{};

std::string CommandDispatcher::handle(const std::string& input) {
    json response;

    try {
        auto j = json::parse(input);
        std::string type = j["type"];
        std::string command = j["command"];
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
                Result result = rep_user.createServer(payload["name"], payload["user_id"], payload["password"]);
                switch (result.getResult()) {
                case Error::None:{
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "ok";
                    response["data"] = ResponseOK{{"answer", "create server: " + payload["name"]}};
                    response["error-msg"] = {};
                    break;
                }
                case Error::ServerAlreadyExists: {
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "error";
                    response["data"] = ResponseOK{};
                    response["error-msg"] = "server name already used";
                    break;
                }
                case Error::DbError:{
                    response["type"] = "response";
                    response["command"] = "createServer";
                    response["status"] = "error";
                    response["data"] = ResponseOK{};
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

            }
            else if(command == "joinToChannel") {

            }
            else if(command == "sendMessageToTextChannel") {
                response["type"] = "response";
                response["command"] = "register";
                response["status"] = "ok";
                response["data"] = ResponseOK{{"answer", "author " + payload["author"] + " says: " + payload["content"]}};
                response["error-msg"] = {};
            }
        }

        return response.dump();
    } catch (...) {
        response["type"] = "response";
        response["command"] = "register";
        response["status"] = "error";
        response["data"] = ResponseOK{};
        response["error-msg"] = "invalid request";
        return response.dump();
    }
}
