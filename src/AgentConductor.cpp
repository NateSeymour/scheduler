#include <iostream>
#include <chrono>
#include <thread>
#include "message/NysMqBroadcaster.h"
#include "AgentConductor.h"
#include "AgentMission.h"
#include "ipc/IpcServer.h"

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

std::filesystem::path AgentConductor::ResolvePath(const std::vector<std::filesystem::path>& paths)
{
    for(auto const& path : paths)
    {
        if(fs::exists(path)) return path;
    }

    throw std::runtime_error("None of the paths exists!");
}

void AgentConductor::RunAgent()
{
    NysMqBroadcaster broadcaster;

    AgentMission mission;
    mission.base = this->launch_parameters.home_directory / ".nys";
    mission.binary = this->launch_parameters.binary_path;
    mission.database_resources = AgentConductor::ResolvePath({
        "/opt/homebrew/share/nys_scheduler/database",
        "/usr/local/share/nys_scheduler/database",
        "../database",
        "./database"
    });
    mission.config = AgentConductor::ResolvePath({
        "/opt/homebrew/etc/nys_scheduler/nys.toml",
         "/usr/local/etc/nys_scheduler/nys.toml",
         "../nys.toml",
         "./nys.toml"
     });
    mission.broadcaster = &broadcaster;

    logger->Log("Conductor launched from `%s`", mission.binary.c_str());
    logger->Log("Using `%s` as mission base.", mission.base.c_str());

    Agent agent(mission);

    logger->Log("Starting server...");
    IpcServer server;
    server.StartServer((mission.base / "nys.sock").c_str());

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