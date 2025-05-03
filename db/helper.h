#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <uuid/uuid.h>

enum class Error {
    None,

    // Регистрация
    EmailAlreadyUsed,
    UsernameAlreadyUsed,

    // Логин
    UserNotFound,
    WrongPassword,

    // Сервер
    ServerAlreadyExists,

    //Каналы
    ChannelAlreadyExists,

    // Системные ошибки
    DbError
};

struct UUID {
    std::string value;

    bool operator==(const UUID& other) const { return value == other.value; }
    operator std::string() const { return value; }
};

template<typename T>
struct Result {
    Error error;
    T value;

    Error getResult() const {return error;};
    bool ok() const { return error == Error::None; }
};

template<>
struct Result<void> {
    Error error;

    Error getResult() const {return error;};
    bool ok() const { return error == Error::None; }
};

UUID generateUuid();
std::string sha256(const std::string &input);

namespace base64 {
static const std::string b64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

inline std::string encode(const unsigned char* bytes_to_encode, size_t in_len) {
    std::string ret;
    int i = 0, j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] =   char_array_3[2] & 0x3f;

            for(i = 0; i < 4; ++i)
                ret += b64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; ++j)
            char_array_3[j] = '\0';
        char_array_4[0] =  (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] =   char_array_3[2] & 0x3f;
        for (j = 0; j < i + 1; ++j)
            ret += b64_chars[char_array_4[j]];
        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

inline std::string decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    size_t i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; ++i)
                char_array_4[i] = b64_chars.find(char_array_4[i]);
            char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];
            for (i = 0; i < 3; ++i)
                ret += char_array_3[i];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j <4; ++j)
            char_array_4[j] = 0;
        for (j = 0; j <4; ++j)
            char_array_4[j] = b64_chars.find(char_array_4[j]);
        char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];
        for (j = 0; j < i - 1; ++j)
            ret += char_array_3[j];
    }
    return ret;
}
}

#endif // HELPER_H
