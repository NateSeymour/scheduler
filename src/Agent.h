#ifndef NOT_YOUR_SCHEDULER_AGENT_H
#define NOT_YOUR_SCHEDULER_AGENT_H

#include <filesystem>
#include <vector>
#include <chrono>
#include <sqlite3.h>
#include "LaunchEnvironment.h"
#include "AgentMission.h"
#include "Unit.h"
#include "Logger.h"
#include "ScheduledTaskQueue.h"
#include "parser/TriggerParser.h"
#include "AgentConductor.h"

class Agent
{
private:
    AgentMission mission;

    TriggerParser trigger_parser;

    sqlite3 *database = nullptr;
    int database_version = -1;

    std::vector<std::shared_ptr<Unit>> units;

    std::unique_ptr<Logger> logger;

    ScheduledTaskQueue low_priority_queue;

    [[noreturn]] void LowPriorityRunner();
    int RunTask(const ScheduledTask& task);
    void UpdateDatabase();
    void LoadUnits();
    void Cleanup();

public:
    int Run();
    explicit Agent(AgentMission mission) : mission(std::move(mission))
    {
        this->logger = std::make_unique<Logger>("Agent", this->mission.base / "log" / "agent.log");
    }
};

#endif //NOT_YOUR_SCHEDULER_AGENT_H
