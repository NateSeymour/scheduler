#include <iostream>
#include <chrono>
#include <thread>
#include "AgentConductor.h"
#include "AgentMission.h"

namespace fs = std::filesystem;

void AgentConductor::VerifyMakeOrFailDirectory(const fs::path& path)
{
    if(!fs::exists(path))
    {
        std::cout << "Didn't find directory at " << path << ". Creating..." << std::endl;

        bool sd_create = fs::create_directories(path);
        if(!sd_create)
        {
            throw std::runtime_error("Failed to create directory.");
        }
    }
}

void AgentConductor::SetupNysDirectory() const
{
    AgentConductor::VerifyMakeOrFailDirectory(this->launch_parameters.home_directory / ".nys");
    AgentConductor::VerifyMakeOrFailDirectory(this->launch_parameters.home_directory / ".nys" / "schedule");
    AgentConductor::VerifyMakeOrFailDirectory(this->launch_parameters.home_directory / ".nys" / "log");
    AgentConductor::VerifyMakeOrFailDirectory(this->launch_parameters.home_directory / ".nys" / "scripts");
    AgentConductor::VerifyMakeOrFailDirectory(this->launch_parameters.home_directory / ".nys" / "workspace");
}

void AgentConductor::Setup()
{
    this->SetupNysDirectory();
    this->logger = std::make_unique<Logger>("AgentConductor", this->launch_parameters.home_directory / ".nys" / "log" / "conductor.log");
}

void AgentConductor::RunAgent()
{
    AgentMission mission;
    mission.base = this->launch_parameters.home_directory / ".nys";
    mission.binary = this->launch_parameters.binary_path;

    logger->Log("Conductor launched from `%s`", mission.binary.c_str());
    logger->Log("Using `%s` as mission base.", mission.base.c_str());

    Agent agent(mission);

    /*
     * If the agent returns, then we allow the program to exit.
     * Otherwise, we attempt to restart.
     */
    while(true)
    {
        try
        {
            logger->Log("Launching agent...");
            agent.Run();
            break;
        }
        catch(std::exception& exception)
        {
            logger->Error("Agent has encountered the following unrecoverable error: %s", exception.what());
            logger->Error("Restarting in 10s...");

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}

