#ifndef REQUEST_REPOSITORIES_H
#define REQUEST_REPOSITORIES_H

#include "postgreshandler.h"
#include "cassandrahandler.h"
#include "helper.h"

struct Friend {
    std::string id;
    std::string username;
    std::string avatar_url;
    std::string status;
};

struct Server {
    std::string id;
    std::string name;
    std::string icon_url;
};

struct Channel {
    std::string id;
    std::string name;
    std::string type;
    std::string server_id;
};

class UserRepository {
public:
    explicit UserRepository(PostgresHandler& handler);
    Result<UUID> registerUser(const std::string& email, const std::string& username, const std::string& password);
    Result<UUID> loginUser(const std::string& email, const std::string& password);
    Result<UUID> leaveServer(const std::string& user_id, const std::string& server_id);
    Result<Server> createServer(const std::string name, const std::string& owner_id,
                              const std::string& password, const std::string& icon_url);
    Result<UUID> deleteServer(const std::string& user_id, const std::string& server_id);
    Result<UUID> joinServer(const std::string& user_id, const std::string name, const std::string& password);
    Result<std::vector<Friend>> getFriends(const std::string& user_id);
    Result<std::vector<Server>> getUserServers(const std::string& user_id);
    Result<UUID> getCurrentUUID(const std::string& username);
private:
    PostgresHandler& handler;
};

class ServerRepository {
public:
    explicit ServerRepository(PostgresHandler& handler);
    Result<UUID> addNewMember(const std::string& user_id);
    Result<UUID> deleteServer(const std::string& server_id);
    Result<UUID> getServer(const std::string name, const std::string& password);
    Result<Channel> createChannel(const std::string& name, const std::string& password,
                               const std::string& type, const std::string& server_id,
                               const std::string& owner_id);
    Result<std::vector<Channel>> getAllChannels(const std::vector<std::string>& server_id);
    Result<std::vector<UUID>> getMembers(const std::string& server_id);

private:
    PostgresHandler& handler;
};


// class ChannelRepository {
//     explicit ChannelRepository(PostgresHandler& handler);
// public:
//     Result<std::vector<Message>> getMessages(const std::string& channelId);

// private:
//     PostgresHandler& handler;
// };

#endif // REQUEST_REPOSITORIES_H
