cmake_minimum_required(VERSION 3.21)
project(not_your_scheduler VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Libs
find_library(SQLite3 NAMES sqlite sqlite3 SQLite3 REQUIRED VERSION 3.39)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v3.2.0
)
FetchContent_MakeAvailable(tomlplusplus)

# Copy supporting files
file(COPY database DESTINATION .)

# parser
add_library(libnys
        src/Agent.cpp
        src/Agent.h
        src/LaunchEnvironment.h
        src/Logger.h
        src/AgentConductor.cpp
        src/AgentConductor.h
        src/AgentMission.h
        src/Unit.h
        src/Unit.cpp
        src/ScheduledTaskQueue.cpp
        src/ScheduledTaskQueue.h
        src/ScheduledTask.cpp
        src/ScheduledTask.h
        src/parser/TriggerParser.cpp
        src/parser/TriggerParser.h
        src/parser/Tokenizer.cpp
        src/parser/Tokenizer.h
        src/parser/Token.h
)
target_link_libraries(libnys PRIVATE tomlplusplus::tomlplusplus SQLite3)

# NYS Daemon
add_executable(nysd src/daemon.cpp src/daemon.h)
target_link_libraries(nysd PUBLIC libnys)

# nysctl
add_executable(nysctl src/nysctl.cpp src/nysctl.h)
target_link_libraries(nysctl PUBLIC libnys)

# libnys tests
add_executable(libnys_test test/parser/TriggerParser.cpp)
target_link_libraries(libnys_test PUBLIC gtest_main libnys)
target_include_directories(libnys_test PRIVATE src)

# GTest integration
include(GoogleTest)
gtest_discover_tests(libnys_test)