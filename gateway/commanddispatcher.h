#ifndef COMMANDDISPATCHER_H
#define COMMANDDISPATCHER_H

#include <string>
#include <map>
#include <db/postgreshandler.h>
#include <db/cassandrahandler.h>

using ResponseOK = std::map<std::string, std::string>;

class CommandDispatcher
{
public:
    static std::string handle(const std::string& command);

private:
    static PostgresHandler psql_handler;
    static CassandraHandler cass_handler;
};

#endif // COMMANDDISPATCHER_H
