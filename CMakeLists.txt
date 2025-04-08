cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(PROJECT_NAME "daily-dosu")
set(CMAKE_BUILD_TYPE Debug)

project(${PROJECT_NAME})
set(SOURCES
    src/main.cpp
    src/DosuConfig.cpp
    src/DailyJob.cpp

    src/bot/Bot.cpp
    src/bot/EmbedGenerator.cpp

    src/database/RankingsDatabase.cpp
    src/database/BotConfigDatabase.cpp
    src/database/TopPlaysDatabase.cpp

    src/http/OsuWrapper.cpp
    src/http/OsutrackWrapper.cpp
    src/http/TokenManager.cpp
    src/http/HttpRequester.cpp

    src/job/ScrapeRankings.cpp
    src/job/GetTopPlays.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Werror
    -Wextra
    -Wunused
    -Wunused-variable
    -Wunused-parameter
    -Wredundant-decls
    -Wno-unused-function
    -Wno-unused-local-typedefs
    -Wconversion
    -Wsign-conversion
    -Wnull-dereference
    -Wdouble-promotion
    -Wformat=2
    -Wcast-align
    -Wcast-qual
    -Wold-style-cast
    -Woverloaded-virtual
    -Wshadow
    -Wno-error=deprecated-declarations
)

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
if(APPLE)
    if(CMAKE_APPLE_SILICON_PROCESSOR)
        set(OPENSSL_ROOT_DIR "/opt/homebrew/opt/openssl")
    else()
        set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
    endif()
    find_package(OpenSSL REQUIRED)
else()
    find_package(OpenSSL REQUIRED)
endif()

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC
    ${CURL_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/bot
    ${CMAKE_CURRENT_SOURCE_DIR}/include/database
    ${CMAKE_CURRENT_SOURCE_DIR}/include/http
    ${CMAKE_CURRENT_SOURCE_DIR}/include/job
)

target_link_libraries(${PROJECT_NAME}
    dl
    ${CMAKE_THREAD_LIBS_INIT}
    ${CURL_LIBRARIES}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    lib/json/include
)

add_subdirectory(lib/DPP)
if(TARGET dpp)
    set_target_properties(dpp PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:dpp,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()
target_link_libraries(${PROJECT_NAME} dpp)

add_subdirectory(lib/SQLiteCpp)
if(TARGET SQLiteCpp)
    set_target_properties(SQLiteCpp PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:SQLiteCpp,INTERFACE_INCLUDE_DIRECTORIES>
    )
endif()
target_link_libraries(${PROJECT_NAME} SQLiteCpp)
