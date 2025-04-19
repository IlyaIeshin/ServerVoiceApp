cmake_minimum_required(VERSION 3.24)
project(VoiceServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/local")

add_subdirectory(core)
add_subdirectory(db)
add_subdirectory(sfu)
add_subdirectory(gateway)
