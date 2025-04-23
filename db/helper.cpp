#include "helper.h"
#include <openssl/sha.h>
#include <iostream>
#include <iomanip>
#include <sstream>

UUID generateUuid()
{
    uuid_t uuid;
    char str[37];
    uuid_generate(uuid);
    uuid_unparse(uuid, str);
    return UUID(str);
}

std::string sha256(const std::string &input) {
    if(input.empty())
        return {};
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)input.c_str(), input.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];

    return ss.str();
}
