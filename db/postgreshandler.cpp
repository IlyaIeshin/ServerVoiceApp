#include "postgreshandler.h"
#include <iostream>

PostgresHandler::PostgresHandler(const std::string &conn_name)
{
    try {
        conn = std::make_unique<pqxx::connection>(conn_name);
        if (!conn->is_open()) {
            std::cerr << "Postgres connection failed.\n";
            last_error = CONNECTION_ERROR;
        }
        else last_error = NONE;
    } catch (const std::exception& e) {
        std::cerr << "Exception during PostgreSQL connection: " << e.what() << "\n";
        last_error = CONNECTION_ERROR;
    }
}

bool PostgresHandler::execute(const std::string& query) {
    try {
        pqxx::work txn(*conn);
        txn.exec(query);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Execute query error: " << e.what() << "\n";
        last_error = EXECUTE_QUERY_ERROR;
        return false;
    }
}


pqxx::result PostgresHandler::query(const std::string& query_str) {
    try {
        pqxx::work txn(*conn);
        pqxx::result res = txn.exec(query_str);
        txn.commit();
        return res;
    } catch (const std::exception& e) {
        std::cerr << "Query execution error: " << e.what() << "\n";
        last_error = EXECUTE_QUERY_ERROR;
        return pqxx::result();
    }
}

ErrorPSQL PostgresHandler::lastError() const {
    return last_error;
}
