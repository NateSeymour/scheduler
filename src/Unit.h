#ifndef NOT_YOUR_SCHEDULER_UNIT_H
#define NOT_YOUR_SCHEDULER_UNIT_H

#include <vector>
#include <string>
#include <filesystem>
#include <chrono>

enum UnitPriority
{
    LOW,
    MEDIUM,
    HIGH
};

enum SchedulingBehavior
{
    BATCH,
    STRICT
};

struct Unit
{
    std::filesystem::path path;
    std::string name;
    std::string description = "Generic description.";
    UnitPriority priority = LOW;
    std::string md5_hash;
    std::string trigger = "never";

    SchedulingBehavior scheduling_behavior = BATCH;

    std::filesystem::path exec;
    std::vector<std::string> arguments;

    std::chrono::time_point<std::chrono::system_clock> last_executed;
};

#endif //NOT_YOUR_SCHEDULER_UNIT_H
