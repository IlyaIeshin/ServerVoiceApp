#ifndef REQUEST_REPOSITORIES_H
#define REQUEST_REPOSITORIES_H

#include "postgreshandler.h"
#include "cassandrahandler.h"
#include "helper.h"

class UserRepository {
public:
    explicit UserRepository(PostgresHandler& handler);
    Result<UUID> registerUser(const std::string& email, const std::string& username, const std::string& password);
    Result<UUID> loginUser(const std::string& email, const std::string& password);
    Result<void> leaveServer(const std::string& user_id, const std::string& server_id);
    Result<UUID> createServer(const std::string name, const std::string& owner_id, const std::string& password);
    Result<UUID> deleteServer(const std::string& user_id, const std::string& server_id);
    Result<UUID> joinServer(const std::string& user_id, const std::string name, const std::string& password);
    Result<std::vector<UUID>> getFriendIds(const std::string& user_id);
    Result<std::vector<UUID>> getUserServerIds(const std::string& user_id);
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
    Result<UUID> createChannel(const std::string name, const std::string& type);
    Result<std::vector<UUID>> getChannelIds(const std::string& server_id);
    Result<std::vector<UUID>> getMemberIds(const std::string& server_id);

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
