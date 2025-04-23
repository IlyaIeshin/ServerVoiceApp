project(VoiceServer)

add_executable(${PROJECT_NAME}
    main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    gateway
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/../local/include
    ${CMAKE_CURRENT_LIST_DIR}/..
)

