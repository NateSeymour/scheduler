#ifndef NOT_YOUR_SCHEDULER_AGENTMISSION_H
#define NOT_YOUR_SCHEDULER_AGENTMISSION_H

#include "../message/NysMqBroadcaster.h"
#include "../Logger.h"
#include "NysConfig.h"

struct AgentMission
{
    NysConfig config;
    NysMqBroadcaster* broadcaster = nullptr;
};

#endif //NOT_YOUR_SCHEDULER_AGENTMISSION_H
