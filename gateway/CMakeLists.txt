project(gateway)

add_library(${PROJECT_NAME} SHARED gateway.h gateway.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

find_package(Crow CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        Crow::Crow
        OpenSSL::SSL
        Threads::Threads
)
