#ifndef NOT_YOUR_SCHEDULER_UNIT_H
#define NOT_YOUR_SCHEDULER_UNIT_H

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <nlohmann/json.hpp>

enum UnitPriority
{
    PRIORITY_LOW,
    PRIORITY_MEDIUM,
    PRIORITY_HIGH
};

enum SchedulingBehavior
{
    SCHEDULING_BATCH,
    SCHEDULING_STRICT
};

struct Unit
{
    std::filesystem::path path;
    std::string name;
    std::string description = "Generic description.";
    UnitPriority priority = PRIORITY_LOW;
    std::string md5_hash;
    std::string trigger = "never";

    SchedulingBehavior scheduling_behavior = SCHEDULING_BATCH;

    std::filesystem::path exec;
    std::vector<std::string> arguments;

    std::chrono::time_point<std::chrono::system_clock> last_executed;

    std::atomic_bool is_running = false;
    std::atomic_uint64_t pid;

    nlohmann::json ToJson();
    static Unit FromJson(const nlohmann::json& json);
};

const char *unit_priority_to_string(UnitPriority priority);
const char *scheduling_behavior_to_string(SchedulingBehavior behavior);

#endif //NOT_YOUR_SCHEDULER_UNIT_H
