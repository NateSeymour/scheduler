#ifndef NOT_YOUR_SCHEDULER_AGENTMISSION_H
#define NOT_YOUR_SCHEDULER_AGENTMISSION_H

#include <filesystem>

struct AgentMission {
    std::filesystem::path base;
    std::filesystem::path binary;
};

#endif //NOT_YOUR_SCHEDULER_AGENTMISSION_H
