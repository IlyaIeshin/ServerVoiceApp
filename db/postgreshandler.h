#ifndef POSTGRESHANDLER_H
#define POSTGRESHANDLER_H

#include <pqxx/pqxx>
#include <memory>

enum ErrorPSQL {
    NONE = 0,
    CONNECTION_ERROR,
    EXECUTE_QUERY_ERROR
};

class PostgresHandler {
public:
    PostgresHandler(const std::string& conn_name);
    bool execute(const std::string& query);
    pqxx::result query(const std::string& query_str);
    ErrorPSQL lastError() const;

private:
    std::unique_ptr<pqxx::connection> conn;
    ErrorPSQL last_error = NONE;
};

#endif // POSTGRESHANDLER_H
