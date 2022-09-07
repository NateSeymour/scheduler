#ifndef NOT_YOUR_SCHEDULER_AGENTMISSION_H
#define NOT_YOUR_SCHEDULER_AGENTMISSION_H

#include <filesystem>
#include "message/NysMqBroadcaster.h"
#include "Logger.h"

struct AgentMission
{
    std::filesystem::path base;
    std::filesystem::path binary;
    std::filesystem::path database_resources;
    std::filesystem::path config;
    NysMqBroadcaster* broadcaster;
};

#endif //NOT_YOUR_SCHEDULER_AGENTMISSION_H
