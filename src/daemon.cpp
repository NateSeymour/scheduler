#include <cstdlib>
#include <filesystem>
#include <vector>
#include <chrono>
#include <thread>
#include <csignal>
#include <toml++/toml.h>
#include "Agent.h"
#include "Logger.h"
#include "daemon.h"
#include "ipc/IpcServer.h"
#include "util/fs.h"

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

int main(int argc, const char **argv)
{
    // Setup mission config
    AgentMission mission;
    mission.base = fs::path(std::getenv("HOME")) / ".nys"; // Default. Overridden by nys.toml
    mission.binary = argv[0];
    mission.database_resources = nys::fs::find_resource("share/database");
    mission.config = nys::fs::find_resource("etc/nys.toml");
    mission.broadcaster = &nys::daemon::broadcaster;

    // Process config file
    auto config = toml::parse_file(mission.config.string());

    if(config.at_path("mission.base").is_string())
    {
        mission.base = config.at_path("mission.base").as_string()->get();
    }

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

    // Start IPC server
    logger.Log("Starting server...");
    IpcServer server;
    server.StartServer(mission);

    /*
     * If the agent returns, then we allow the program to exit.
     * Otherwise, we attempt to restart.
     */
    while(true)
    {
        // Configure agent
        Agent agent(mission);

        try
        {
            // Start agent
            logger.Log("Launching agent...");

            // Blocking
            AgentReturn ret = agent.Run();

            if(ret == AGENT_RELOAD)
            {
                logger.Log("Reloading agent after graceful shutdown...");
                continue;
            }

            return 0;
        }
        catch(std::exception& exception)
        {
            // Ensure resources are deallocated
            agent.Cleanup();

            logger.Error("Agent has encountered the following unrecoverable error: %s", exception.what());
            logger.Error("Restarting in 10s...");

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}