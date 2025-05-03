#include "cassandrahandler.h"
#include <iostream>
#include "helper.h"

CassandraHandler::CassandraHandler(const std::string& hosts,
                                   unsigned short port,
                                   const std::string& user,
                                   const std::string& password)
    : cluster_(cass_cluster_new(), cass_cluster_free),
    session_(cass_session_new(), cass_session_free) {

    cass_cluster_set_contact_points(cluster_.get(), hosts.c_str());
    cass_cluster_set_port(cluster_.get(), port);

    if (!user.empty())
        cass_cluster_set_credentials(cluster_.get(), user.c_str(), password.c_str());

    CassFuture* f = cass_session_connect(session_.get(), cluster_.get());
    cass_future_wait(f);

    if (cass_future_error_code(f) != CASS_OK) {
        const char* msg; size_t len;
        cass_future_error_message(f, &msg, &len);
        std::cerr << "Cassandra connect error: "
                  << std::string(msg, len) << std::endl;
        last_error = CASS_CONNECTION_ERROR;
    } else {
        last_error = CASS_NONE;
    }
    cass_future_free(f);
}

CassandraHandler::~CassandraHandler() {
    if (session_) {
        CassFuture* close_future = cass_session_close(session_.get());
        cass_future_wait(close_future);
        cass_future_free(close_future);
    }
}

bool CassandraHandler::execute(const std::string& cql) {
    CassStatement* st = cass_statement_new(cql.c_str(), 0);
    CassFuture* f     = cass_session_execute(session_.get(), st);
    cass_statement_free(st);

    CassError rc = cass_future_error_code(f);
    cass_future_free(f);

    if (rc != CASS_OK) {
        std::cerr << "Cassandra exec error: "
                  << cass_error_desc(rc) << std::endl;
        last_error = CASS_EXECUTE_QUERY_ERROR;
        return false;
    }
    last_error = CASS_NONE;
    return true;
}

std::optional<CassResultPtr> CassandraHandler::query(const std::string& cql) {
    CassStatement* st = cass_statement_new(cql.c_str(), 0);
    CassFuture* f     = cass_session_execute(session_.get(), st);

    cass_statement_free(st);
    cass_future_wait(f);

    CassError rc = cass_future_error_code(f);
    if (rc != CASS_OK) {
        cass_future_free(f);
        last_error = CASS_EXECUTE_QUERY_ERROR;
        return std::nullopt;
    }

    CassResultPtr res(cass_future_get_result(f), cass_result_free);

    cass_future_free(f);
    last_error = CASS_NONE;
    return res;
}

std::pair<cass::ResultView, std::string>
CassandraHandler::pagedQuery(const std::string&  cql,
                             const std::vector<std::string>& params,
                             const std::string&  pageState,
                             int pageSize)
{
    /* 1. Statement ------------------------------------------------------ */
    CassStatement* stmt = cass_statement_new(
        cql.c_str(),
        static_cast<cass_uint32_t>(params.size()));

    /* 1a. Биндим каждый параметр */
    for (cass_uint32_t i = 0; i < params.size(); ++i) {
        CassUuid uuid;
        if (cass_uuid_from_string(params[i].c_str(), &uuid) == CASS_OK) {
            cass_statement_bind_uuid(stmt, i, uuid);          // UUID
        } else {
            cass_statement_bind_string(stmt, i, params[i].c_str()); // TEXT
        }
    }

    cass_statement_set_paging_size(
        stmt, static_cast<cass_uint32_t>(pageSize));

    /* 2. Передаём токен предыдущей страницы ----------------------------- */
    if (!pageState.empty()) {
        std::string raw = base64::decode(pageState);
        cass_statement_set_paging_state_token(
            stmt, raw.data(), raw.size());
    }

    /* 3. Выполняем ------------------------------------------------------- */
    CassFuture* fut = cass_session_execute(session_.get(), stmt);
    cass_future_wait(fut);

    if (cass_future_error_code(fut) != CASS_OK) {
        const char* msg;  size_t len;
        cass_future_error_message(fut, &msg, &len);
        std::string err(msg, len);
        cass_future_free(fut);  cass_statement_free(stmt);
        throw std::runtime_error("pagedQuery failed: " + err);
    }

    const CassResult* rawRes = cass_future_get_result(fut);

    /* 4. Оборачиваем в ResultView (RAII) -------------------------------- */
    CassResultPtr resPtr(rawRes, &cass_result_free);
    cass::ResultView rv(resPtr);

    /* 5. Токен следующей страницы --------------------------------------- */
    std::string nextPage;
    if (cass_result_has_more_pages(rawRes)) {
        const char* tokPtr = nullptr; size_t tokSize = 0;
        if (cass_result_paging_state_token(rawRes, &tokPtr, &tokSize) == CASS_OK &&
            tokPtr && tokSize)
        {
            nextPage = base64::encode(
                reinterpret_cast<const unsigned char*>(tokPtr), tokSize);
        }
    }

    /* 6. Чистим ---------------------------------------------------------- */
    cass_future_free(fut);
    cass_statement_free(stmt);

    return { std::move(rv), std::move(nextPage) };
}
