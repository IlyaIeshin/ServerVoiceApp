cmake_minimum_required(VERSION 3.24)
project(VoiceServerALL LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/local")

add_subdirectory(db)
add_subdirectory(sfu)
add_subdirectory(gateway)
add_subdirectory(core)

