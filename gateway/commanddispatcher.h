#ifndef COMMANDDISPATCHER_H
#define COMMANDDISPATCHER_H

#include <string>
#include <map>
#include <db/postgreshandler.h>
#include <db/cassandrahandler.h>
#include <db/request_repositories.h>
#include <nlohmann/json.hpp>
#include <sfu/janushandler.h>
#include <crow.h>
#include <shared_mutex>

extern std::unordered_map<
    std::string,                       // channel_id
    std::unordered_map<std::string,    // user_id
                       nlohmann::json> // user_json
    > voiceMembers;

extern std::shared_mutex voiceMtx;

extern std::unordered_map<
    crow::websocket::connection*,      // conn*
    std::pair<std::string,std::string> // {channel_id,user_id}
    > connVoiceMap;


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
