cmake_minimum_required(VERSION 3.21)
project(not_your_scheduler VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 20)

# Libs
find_library(SQLite3 NAMES sqlite sqlite3 SQLite3 REQUIRED VERSION 3.39)
add_subdirectory(lib/tomlplusplus)

# Copy supporting files
file(COPY database DESTINATION .)

add_executable(nys src/main.cpp src/main.h src/Agent.cpp src/Agent.h src/LaunchEnvironment.h src/Logger.h src/AgentConductor.cpp src/AgentConductor.h src/AgentMission.h src/Unit.h src/Unit.cpp src/ScheduledTaskQueue.cpp src/ScheduledTaskQueue.h src/ScheduledTask.cpp src/ScheduledTask.h src/Time.cpp src/Time.h src/parser/TriggerParser.cpp src/parser/TriggerParser.h)
target_link_libraries(nys PRIVATE tomlplusplus::tomlplusplus SQLite3)