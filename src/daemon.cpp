#include <cstdlib>
#include <filesystem>
#include <vector>
#include <chrono>
#include <thread>
#include <csignal>
#include "Agent.h"
#include "Logger.h"
#include "daemon.h"
#include "ipc/IpcServer.h"

namespace fs = std::filesystem;

void signal_handler(int signum)
{
    switch(signum)
    {
        case SIGTERM:
        {
            NysMessage shutdown_message {
                .type = MESSAGE_SHUTDOWN
            };
            nys::daemon::broadcaster.Broadcast(shutdown_message);
        }

        default:
            break;
    }
}

fs::path resolve_path(const std::vector<fs::path>& paths)
{
    for(auto const& path : paths)
    {
        if(fs::exists(path)) return path;
    }

    throw std::runtime_error("None of the paths exists!");
}

int main(int argc, const char **argv)
{
    // TODO: process config file
    // Setup mission config
    AgentMission mission;
    mission.base = fs::path(std::getenv("HOME")) / ".nys";
    mission.binary = argv[0];
    mission.database_resources = resolve_path({
             "/opt/homebrew/share/nys_scheduler/database",
             "/usr/local/share/nys_scheduler/database",
             "../database",
             "./database"
    });
    mission.config = resolve_path({
             "/opt/homebrew/etc/nys_scheduler/nys.toml",
             "/usr/local/etc/nys_scheduler/nys.toml",
             "../nys.toml",
             "./nys.toml"
    });
    mission.broadcaster = &nys::daemon::broadcaster;

    // Create directories
    std::vector<fs::path> required_dirs = {
            mission.base,
            mission.base / "log",
            mission.base / "schedule",
            mission.base / "scripts",
            mission.base / "workspace"
    };

    for(auto const& dir : required_dirs)
    {
        if(!fs::exists(dir))
        {
            fs::create_directories(dir);
        }
    }

    // Create logger
    Logger logger("Daemon", mission.base / "log" / "daemon.log");

    logger.Log("Daemon launched from `%s`", mission.binary.c_str());
    logger.Log("Using `%s` as mission base.", mission.base.c_str());

    // Register signal handler
    std::signal(SIGTERM, signal_handler);

    // Configure agent
    Agent agent(mission);

    // Start IPC server
    logger.Log("Starting server...");
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
            logger.Log("Launching agent...");
            agent.Run();
            return 0;
        }
        catch(std::exception& exception)
        {
            // Free any still-allocated resources
            agent.Cleanup();

            logger.Error("Agent has encountered the following unrecoverable error: %s", exception.what());
            logger.Error("Restarting in 10s...");

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}