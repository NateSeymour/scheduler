#ifndef NOT_YOUR_SCHEDULER_AGENTCONDUCTOR_H
#define NOT_YOUR_SCHEDULER_AGENTCONDUCTOR_H

#include <mutex>
#include <filesystem>
#include <vector>
#include "LaunchEnvironment.h"
#include "Agent.h"
#include "Logger.h"

class AgentConductor
{
private:
    LaunchEnvironment launch_parameters;

    std::unique_ptr<Logger> logger;

    static void VerifyMakeOrFailDirectory(const std::filesystem::path& path);
    static std::filesystem::path ResolvePath(const std::vector<std::filesystem::path>& paths);
    void SetupNysDirectory() const;

    friend class Agent;

public:
    void Setup();

    void RunAgent();
    explicit AgentConductor(LaunchEnvironment launch_parameters) : launch_parameters(std::move(launch_parameters)) {}
};

#endif //NOT_YOUR_SCHEDULER_AGENTCONDUCTOR_H
