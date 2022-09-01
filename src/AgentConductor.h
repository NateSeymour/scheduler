#ifndef NOT_YOUR_SCHEDULER_AGENTCONDUCTOR_H
#define NOT_YOUR_SCHEDULER_AGENTCONDUCTOR_H

#include <mutex>
#include <filesystem>
#include "LaunchEnvironment.h"
#include "Agent.h"

class AgentConductor
{
private:
    LaunchEnvironment launch_parameters;

    static void VerifyMakeOrFailDirectory(const std::filesystem::path& path);
    void SetupNysDirectory() const;

    friend class Agent;

public:
    void Setup();

    Agent NewAgent();
    explicit AgentConductor(LaunchEnvironment launch_parameters) : launch_parameters(std::move(launch_parameters)) {}
};

#endif //NOT_YOUR_SCHEDULER_AGENTCONDUCTOR_H
