#ifndef CASSANDRAHANDLER_H
#define CASSANDRAHANDLER_H

#include <cassandra.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "cassandra_iterator.h"

using CassResultPtr = std::shared_ptr<const CassResult>;

enum ErrorCassandra {
    CASS_NONE = 0,
    CASS_CONNECTION_ERROR,
    CASS_EXECUTE_QUERY_ERROR
};

class CassandraHandler {
public:
    explicit CassandraHandler(const std::string& hosts,
                              unsigned short port = 9042,
                              const std::string& user = {},
                              const std::string& password = {});

    ~CassandraHandler();

    bool isReady() const { return last_error == CASS_NONE; }

    bool execute(const std::string& cql);
    std::optional<CassResultPtr> query(const std::string& cql);

    ErrorCassandra lastError() const { return last_error; }

    CassSession* session() const { return session_.get(); }

    std::pair<cass::ResultView, std::string>
    pagedQuery(const std::string& cql,
               const std::vector<std::string>& params,
               const std::string& pageState = "",
               int pageSize = 50);


private:
    std::unique_ptr<CassCluster, decltype(&cass_cluster_free)> cluster_;
    std::unique_ptr<CassSession, decltype(&cass_session_free)> session_;

    ErrorCassandra last_error = CASS_NONE;

    CassandraHandler(const CassandraHandler&) = delete;
    CassandraHandler& operator=(const CassandraHandler&) = delete;
};

#endif // CASSANDRAHANDLER_H
