project(gateway)

add_library(${PROJECT_NAME} SHARED
    gateway.h gateway.cpp
    commanddispatcher.h commanddispatcher.cpp
    jwtservice.h jwtservice.cpp
    helper_gateway.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    CROW_ENABLE_WEBSOCKET
    CROW_ENABLE_COMPRESSION
    CROW_ENABLE_SSL
)

find_package(Crow CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)

file(COPY ${CMAKE_SOURCE_DIR}/certificates/
     DESTINATION ${CMAKE_BINARY_DIR}/core/certificates)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Crow::Crow
        OpenSSL::SSL
        Threads::Threads
        ZLIB::ZLIB
        db
        sfu
)
