#ifndef NOT_YOUR_SCHEDULER_AGENT_H
#define NOT_YOUR_SCHEDULER_AGENT_H

#include <filesystem>
#include <vector>
#include <chrono>
#include <sqlite3.h>
#include "config/AgentMission.h"
#include "Unit.h"
#include "Logger.h"
#include "ScheduledTaskQueue.h"
#include "parser/TriggerParser.h"

enum AgentReturn
{
    AGENT_RELOAD,
    AGENT_SHUTDOWN
};

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

    std::atomic_bool is_running = false;

    void LowPriorityRunner();
    int RunTask(const ScheduledTask& task);
    void UpdateDatabase();
    void LoadUnits();

public:
    AgentReturn Run();
    void Cleanup();
    explicit Agent(AgentMission mission) : mission(std::move(mission))
    {
        this->logger = std::make_unique<Logger>("Agent", this->mission.config.base / "log" / "agent.log");
    }
    ~Agent();
};

#endif //NOT_YOUR_SCHEDULER_AGENT_H
