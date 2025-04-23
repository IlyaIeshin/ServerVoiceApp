#include "jwtservice.h"
#include <jwt-cpp/jwt.h>
#include <stdexcept>

JwtService::JwtService(const std::string& secret)
    : secret_(secret) {}

std::string JwtService::generateToken(const std::string& user_id, int expires_hours) const {
    return jwt::create()
    .set_type("JWT")
        .set_issuer("voiceapp")
        .set_payload_claim("user_id", jwt::claim(user_id))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(expires_hours))
        .sign(jwt::algorithm::hs256{secret_});
}

std::string JwtService::verifyToken(const std::string& token) const {
    auto decoded = jwt::decode(token);

    jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{secret_})
        .with_issuer("voiceapp")
        .verify(decoded);

    return decoded.get_payload_claim("user_id").as_string();
}

