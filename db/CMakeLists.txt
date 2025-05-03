project(db)

find_library(PQXX_LIB pqxx REQUIRED)
find_package(Threads REQUIRED)
set(CASS_DRIVER_PREFIX "${PROJECT_SOURCE_DIR}/../local" CACHE PATH "prefix of manually installed cassandra‑cpp‑driver")

find_path(CASSANDRA_INCLUDE_DIR
          NAMES cassandra.h
          PATHS "${CASS_DRIVER_PREFIX}/include" NO_DEFAULT_PATH)

find_library(CASSANDRA_LIBRARY
             NAMES cassandra
             PATHS "${CASS_DRIVER_PREFIX}/lib" NO_DEFAULT_PATH)

if (NOT CASSANDRA_INCLUDE_DIR OR NOT CASSANDRA_LIBRARY)
    message(FATAL_ERROR "Cassandra C++ driver not found in ${CASS_DRIVER_PREFIX}.\n"
                        "Проверьте, что выполнили 'cmake --install' для cpp‑driver, "
                        "и в каталоге local/lib есть libcassandra.so, а в local/include — cassandra.h")
endif()

add_library(cassandra::cassandra SHARED IMPORTED)
set_target_properties(cassandra::cassandra PROPERTIES
    IMPORTED_LOCATION            "${CASSANDRA_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CASSANDRA_INCLUDE_DIR}")# 4. Минимальные модули-пустышки (библиотеки) — будут наполняться кодом позднее

add_library(${PROJECT_NAME} SHARED postgreshandler.h postgreshandler.cpp
    request_repositories.h request_repositories.cpp
    cassandrahandler.h cassandrahandler.cpp
    helper.h
    helper.cpp
    cassandra_iterator.h
)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        cassandra::cassandra
        #libpqxx::pqxx
        ${PQXX_LIB}
        Threads::Threads
        uuid
)
