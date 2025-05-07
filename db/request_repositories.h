#ifndef REQUEST_REPOSITORIES_H
#define REQUEST_REPOSITORIES_H

#include "postgreshandler.h"
#include "cassandrahandler.h"
#include "helper.h"

struct User {
    std::string id;
    std::string username;
    std::string avatar_url;
};

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

struct Message
{
    std::string message_id;
    std::string channel_id;
    std::string sender_id;
    std::string text;
    std::string created_at;
};

struct PagedMessages {
    std::vector<Message> msgs;
    std::string nextPageState;
};

class UserRepository {
public:
    explicit UserRepository(PostgresHandler& handler);
    Result<User> getUserData(const std::string user_id);
    Result<User> registerUser(const std::string& email, const std::string& username, const std::string& password, const std::string& avatar_url);
    Result<User> loginUser(const std::string& email, const std::string& password);
    Result<UUID> leaveServer(const std::string& user_id, const std::string& server_id);
    Result<Server> createServer(const std::string name, const std::string& owner_id,
                              const std::string& password, const std::string& icon_url);
    Result<Server> joinToServer(const std::string& user_id, const std::string& server_id);
    Result<std::vector<Friend>> getFriends(const std::string& user_id);
    Result<std::vector<Server>> getUserServers(const std::string& user_id);

private:
    PostgresHandler& handler;
};

class ServerRepository {
public:
    explicit ServerRepository(PostgresHandler& handler);
    Result<UUID> deleteServer(const std::string& server_id);
    Result<Channel> createChannel(const std::string& name, const std::string& password,
                               const std::string& type, const std::string& server_id,
                               const std::string& owner_id);
    Result<std::vector<Channel>> getAllChannels(const std::vector<std::string>& server_id);
    Result<std::vector<UUID>> getMembers(const std::string& server_id);

private:
    PostgresHandler& handler;
};


class ChannelRepository {
public:
    explicit ChannelRepository(PostgresHandler& psql_handler, CassandraHandler& cass_hander);
    Result<PagedMessages> getMessagesPage(const std::string& channel_id,const std::string& pageState = "");
    Result<Message> addMessage(const std::string& sender_id, const std::string& text, const std::string& channel_id);

private:
    PostgresHandler& handlerPsql;
    CassandraHandler& handlerCass;
};

#endif // REQUEST_REPOSITORIES_H
