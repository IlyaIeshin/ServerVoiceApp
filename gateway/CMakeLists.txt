project(gateway)

add_library(${PROJECT_NAME} SHARED
    gateway.h gateway.cpp
    commanddispatcher.h commanddispatcher.cpp
    jwtservice.h jwtservice.cpp
)

target_compile_definitions(gateway PRIVATE
    CROW_ENABLE_WEBSOCKET    # если вам нужен WebSocket
    CROW_ENABLE_COMPRESSION  # включаем компрессию
)

find_package(Crow CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)  # <-- добавляем поиск zlib

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Crow::Crow
        OpenSSL::SSL
        Threads::Threads
        ZLIB::ZLIB             # <-- добавляем линковку
        db
)
