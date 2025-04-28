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

Result<Server> UserRepository::createServer(const std::string name, const std::string &owner_id,
                                          const std::string &password, const std::string& icon_url)
{
    try {
        auto checkServerIfExists = handler.query("SELECT id FROM servers WHERE name = '" + name + "'");
        if(!checkServerIfExists.empty()) return {Error::ServerAlreadyExists};

        std::string uuid_server = generateUuid();
        std::string insert = "INSERT INTO servers (id, name, password, owner_id, icon_url) VALUES ('" +
                             uuid_server + "', '" + name + "', '" + sha256(password) + "', '" + owner_id + "', '" + icon_url + "')";
        handler.execute(insert);

        insert = "INSERT INTO server_membership (server_id, user_id, role) VALUES ('" +
                 uuid_server + "', '" + owner_id + "', 'owner')";
        handler.execute(insert);

        return {Error::None, Server{uuid_server, name, icon_url}};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::deleteServer(const std::string &user_id, const std::string &server_id)
{
    try {

    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::leaveServer(const std::string &user_id, const std::string &server_id)
{
    //TODO: необходима проверка на то что user_id == owner_id
    try {
    std::string delete_server = "DELETE FROM server_membership "
                                "WHERE user_id = '" + user_id + "' "
                                "AND server_id = '" + server_id + "'";
    handler.execute(delete_server);
    return {Error::None, {server_id}};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<UUID> UserRepository::joinServer(const std::string &user_id, const std::string name, const std::string &password)
{
    return {};
}

Result<std::vector<Friend> > UserRepository::getFriends(const std::string &user_id)
{
    try {
        std::string select = "SELECT users.id, users.username, users.avatar_url, users.status "
                             "FROM users "
                             "JOIN relations_users ON users.id = relations_users.friend_id "
                             "WHERE relations_users.user_id = '" + user_id + "'";
        auto results = handler.query(select);
        std::vector<Friend> friends;
        for(const auto& res : results) {
            friends.push_back({
                res["id"].as<std::string>(),
                res["username"].as<std::string>(),
                res["avatar_url"].is_null() ? "" : res["avatar_url"].as<std::string>(),
                res["status"].is_null() ? "" : res["status"].as<std::string>()
            });
        }
        return {Error::None, friends};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<std::vector<Server> > UserRepository::getUserServers(const std::string &user_id)
{
    try {
        std::string select = "SELECT servers.id, servers.name, servers.icon_url "
                             "FROM servers "
                             "JOIN server_membership ON servers.id = server_membership.server_id "
                             "WHERE server_membership.user_id = '" + user_id + "'";
        auto results = handler.query(select);
        std::vector<Server> servers;
        for(const auto& res : results) {
            servers.push_back({
                res["id"].as<std::string>(),
                res["name"].as<std::string>(),
                res["icon_url"].is_null() ? "" : res["icon_url"].as<std::string>()
            });
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

Result<Channel> ServerRepository::createChannel(const std::string& name, const std::string& password,
                                             const std::string& type, const std::string& server_id,
                                             const std::string& owner_id)
{
    try {
        auto checkChannelIfExists = handler.query("SELECT id FROM channels WHERE "
                             "name = '" + name + "'");
        if(!checkChannelIfExists.empty()) return {Error::ChannelAlreadyExists};

        std::string uuid = generateUuid();

        std::string insert = "INSERT INTO channels (id, name, password, type, server_id, owner_id) VALUES "
                             "('" + uuid + "', '" + name + "', '" + sha256(password) + "', '" + type +
                             + "', '" + server_id + "', '"  + owner_id + "')";

        handler.execute(insert);

        return {Error::None, Channel{uuid, name, type}};
    } catch (...) {
        return {Error::DbError};
    }
}

Result<std::vector<Channel> > ServerRepository::getAllChannels(const std::vector<std::string> &server_ids)
{
    try {
        if (server_ids.empty()) {
            return {Error::None, {}}; // если список пустой — сразу вернуть пусто
        }

        std::string query = "SELECT id, name, type, server_id FROM channels WHERE server_id = ANY('{";
        for (size_t i = 0; i < server_ids.size(); ++i) {
            query += "\"" + server_ids[i] + "\"";
            if (i != server_ids.size() - 1) {
                query += ",";
            }
        }
        query += "}')";

        auto results = handler.query(query);

        std::vector<Channel> channels;
        for(const auto& res : results) {
            channels.push_back({
                res["id"].as<std::string>(),
                res["name"].as<std::string>(),
                res["type"].as<std::string>(),
                res["server_id"].as<std::string>()
            });
        }
        return {Error::None, channels};
    } catch (...) {
        return {Error::DbError};
    }
    return {};
}

Result<std::vector<UUID> > ServerRepository::getMembers(const std::string &server_id)
{
    return {Error::DbError};
}
