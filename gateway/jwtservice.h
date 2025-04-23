#ifndef JWTSERVICE_H
#define JWTSERVICE_H

#include <string>

class JwtService {
public:
    explicit JwtService(const std::string& secret);

    std::string generateToken(const std::string& user_id, int expires_hours = 24*365) const;
    std::string verifyToken(const std::string& token) const;

private:
    std::string secret_;
};

#endif // JWTSERVICE_H
