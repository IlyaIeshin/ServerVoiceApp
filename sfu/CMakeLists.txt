project(sfu)

add_library(${PROJECT_NAME} SHARED voicesfumanager.h voicesfumanager.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

find_package(LibDataChannel CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        LibDataChannel::LibDataChannel
        OpenSSL::SSL
        Threads::Threads
)
