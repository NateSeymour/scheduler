#include "Unit.h"

nlohmann::json Unit::ToJson()
{
    nlohmann::json unit_json = {
            {"path", this->path},
            {"name", this->name},
            {"description", this->description},
            {"priority", unit_priority_to_string(this->priority)},
            {"md5_hash", this->md5_hash},
            {"trigger", this->trigger},
            {"scheduling_behavior", scheduling_behavior_to_string(this->scheduling_behavior)},
            {"exec", this->exec},
            {"arguments", this->arguments},
            {"is_running", this->is_running.load()},
            {"pid", this->pid.load()}
    };

    return std::move(unit_json);
}

Unit Unit::FromJson(const nlohmann::json &json)
{
    return Unit();
}

const char *unit_priority_to_string(UnitPriority priority)
{
    switch(priority)
    {
        case PRIORITY_LOW:
            return "LOW";
        case PRIORITY_MEDIUM:
            return "MEDIUM";
        case PRIORITY_HIGH:
            return "HIGH";
    }
}

const char *scheduling_behavior_to_string(SchedulingBehavior behavior)
{
    switch(behavior)
    {
        case SCHEDULING_BATCH:
            return "BATCH";
        case SCHEDULING_STRICT:
            return "STRICT";
    }
}
