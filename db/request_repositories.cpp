#include "request_repositories.h"
#include <openssl/sha.h>

UserRepository::UserRepository(PostgresHandler &handler_) : handler(handler_) {}

Result<UUID> UserRepository::registerUser(const std::string &email, const std::string &username, const std::string &password)
{
    try {
        auto emailCheck = handler.query("SELECT id FROM users WHERE email = '" + email + "'");
        if (!emailCheck.empty()) return {Error::EmailAlreadyUsed};

        auto nameCheck = handler.query("SELECT id FROM users WHERE username = '" + username + "'");
        if (!nameCheck.empty()) return {Error::UsernameAlreadyUsed};

        std::string uuid = generateUuid();

        std::string insert = "INSERT INTO users (id, email, username, password_hash) VALUES ('" +
                             uuid + "', '" + email + "', '" + username + "', '" + sha256(password) + "')";

        handler.execute(insert);

        return {Error::None, uuid};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::loginUser(const std::string &email, const std::string &password)
{
    try {
        auto res = handler.query("SELECT id, password_hash FROM users WHERE email = '" + email + "'");
        if (res.empty()) return {Error::UserNotFound};

        auto storedHash = res[0]["password_hash"].as<std::string>();
        if (storedHash != sha256(password)) return {Error::WrongPassword};

        std::string uuid = res[0]["id"].as<std::string>();

        return {Error::None, uuid};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::createServer(const std::string name, const std::string &owner_id, const std::string &password)
{
    try {
        auto checkServerIfExists = handler.query("SELECT id FROM servers WHERE name = '" + name + "'");
        if(!checkServerIfExists.empty()) return {Error::ServerAlreadyExists};

        std::string uuid_server = generateUuid();
        std::string insert = "INSERT INTO servers (id, name, password, owner_id) VALUES ('" +
                             uuid_server + "', '" + name + "', '" + sha256(password) + "', '" + owner_id + "')";
        handler.execute(insert);

        insert = "INSERT INTO server_membership (server_id, user_id, role) VALUES ('" +
                 uuid_server + "', '" + owner_id + "', 'owner')";
        handler.execute(insert);

        return {Error::None, uuid_server};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<void> UserRepository::leaveServer(const std::string &user_id, const std::string &server_id)
{
    //TODO: необходима проверка на то что user_id == owner_id
    try {
    std::string delete_server = "DELETE FROM server_membership "
                                "WHERE user_id = '" + user_id + "' "
                                "AND server_id = '" + server_id + "'";
    handler.execute(delete_server);
    return {Error::None};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::joinServer(const std::string &user_id, const std::string name, const std::string &password)
{
    return {};
}

Result<std::vector<UUID> > UserRepository::getFriendIds(const std::string &user_id)
{
    return {};
}

Result<std::vector<UUID> > UserRepository::getUserServerIds(const std::string &user_id)
{
    try {
        std::string select = "SELECT server_id FROM server_membership WHERE "
                             "user_id = '" + user_id + "'";
        auto results = handler.query(select);
        std::vector<UUID> servers;
        for(auto res : results) {
            servers.push_back(UUID{res["id"].as<std::string>()});
        }
        return {Error::None, servers};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::getCurrentUUID(const std::string& username)
{
    auto result = handler.query("SELECT id FROM users WHERE username = '" + username + "'");
    std::string uuid = result[0]["id"].as<std::string>();
    return {Error::None, uuid};
}

ServerRepository::ServerRepository(PostgresHandler &handler_) : handler(handler_) {}

Result<UUID> ServerRepository::deleteServer(const std::string &server_id)
{
    return {};
}

Result<UUID> ServerRepository::createChannel(const std::string name, const std::string &type)
{
    return {};
}

Result<std::vector<UUID> > ServerRepository::getChannelIds(const std::string &server_id)
{
    return {};
}

Result<std::vector<UUID> > ServerRepository::getMemberIds(const std::string &server_id)
{
    try {
        std::string select = "SELECT server_id FROM server_membership WHERE "
                             "server_id = '" + server_id + "'";
        auto results = handler.query(select);
        std::vector<UUID> users;
        for(auto res : results) {
            users.push_back(UUID{res["id"].as<std::string>()});
        }
        return {Error::None, users};
    } catch (...) {
        return {Error::DbError};
    }
    return {};
}
