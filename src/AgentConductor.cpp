#include <iostream>
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
}

void AgentConductor::Setup()
{
    this->SetupNysDirectory();
}

Agent AgentConductor::NewAgent()
{
    AgentMission mission;
    mission.base = this->launch_parameters.home_directory / ".nys";
    mission.binary = this->launch_parameters.binary_path;

    return Agent(mission);
}
