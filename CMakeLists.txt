cmake_minimum_required(VERSION 3.21)
project(not_your_scheduler VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

# Libs
find_package(SQLite3 3.3 REQUIRED)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
option(INSTALL_GTEST "Enable installation of googletest." OFF)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG        v3.2.0
)
FetchContent_MakeAvailable(tomlplusplus)

# Copy supporting files
configure_file(database/database.toml database/database.toml COPYONLY)
configure_file(database/VERSION-1.sql database/VERSION-1.sql COPYONLY)

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
target_link_libraries(libnys PRIVATE tomlplusplus::tomlplusplus SQLite::SQLite3)

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

# NYS
add_custom_target(nys-scheduler)
add_dependencies(nys-scheduler nysd nysctl libnys_test)

# Install
install(TARGETS nysd nysctl libnys)