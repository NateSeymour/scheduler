cmake_minimum_required(VERSION 3.21)
project(not_your_scheduler VERSION 0.0.3)

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

# parser
add_library(nys
        src/Agent.cpp
        src/Agent.h
        src/Logger.h
        src/config/AgentMission.h
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
        src/ipc/IpcServer.cpp
        src/ipc/IpcServer.h
        src/message/NysMqBroadcaster.cpp
        src/message/NysMqBroadcaster.h
        src/message/NysMqReceiver.cpp
        src/message/NysMqReceiver.h
        src/message/NysMessage.h
        src/daemon.h
        src/ipc/IpcHeader.h
        src/ipc/IpcClient.cpp
        src/ipc/IpcClient.h
        src/config/NysConfig.h
        src/util/fs.cpp
        src/util/fs.h)
target_link_libraries(nys PUBLIC tomlplusplus::tomlplusplus PRIVATE SQLite::SQLite3)

# NYS Daemon
add_executable(nysd src/daemon.cpp)
target_link_libraries(nysd PUBLIC nys)

# nysctl
add_executable(nysctl src/nysctl.cpp)
target_link_libraries(nysctl PUBLIC nys ncurses)

# libnys tests
add_executable(libnys_test test/parser/TriggerParser.cpp)
target_link_libraries(libnys_test PUBLIC gtest_main nys)
target_include_directories(libnys_test PRIVATE src)

# NYS
add_custom_target(nys-scheduler)
add_dependencies(nys-scheduler nysd nysctl libnys_test)

# plist for launchd
configure_file(com.notyoursoftware.scheduler.plist.in ${CMAKE_BINARY_DIR}/com.notyoursoftware.scheduler.plist)

# Install
install(TARGETS nysd nysctl nys)
install(DIRECTORY database DESTINATION share/nys_scheduler)
install(FILES nys.toml DESTINATION etc/nys_scheduler)

if(APPLE)
    install(FILES ${CMAKE_BINARY_DIR}/com.notyoursoftware.scheduler.plist DESTINATION $ENV{HOME}/Library/LaunchAgents)
endif()