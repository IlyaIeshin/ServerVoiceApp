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

#endif // HELPER_H
