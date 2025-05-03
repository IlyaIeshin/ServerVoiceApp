#ifndef COMMANDDISPATCHER_H
#define COMMANDDISPATCHER_H

#include <string>
#include <map>
#include <db/postgreshandler.h>
#include <db/cassandrahandler.h>
#include <db/request_repositories.h>
#include <nlohmann/json.hpp>
#include <crow.h>

using ResponseOK = std::map<std::string, std::string>;

class CommandDispatcher
{
public:
    static nlohmann::json handle(
        crow::websocket::connection& conn,
        const nlohmann::json& req);

private:
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static PostgresHandler psql_handler;
    static CassandraHandler cass_handler;
};

#endif // COMMANDDISPATCHER_H
