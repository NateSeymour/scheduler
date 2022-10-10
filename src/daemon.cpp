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
                .type = MESSAGE_SHUTDOWN,
                .value = {
                        {"reason", "SIGTERM"}
                }
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
    mission.config = NysConfig::FromEnv(argc, argv);
    mission.broadcaster = &nys::daemon::broadcaster;

    // Create directories
    std::vector<fs::path> required_dirs = {
            mission.config.base,
            mission.config.base / "log",
            mission.config.base / "schedule",
            mission.config.base / "scripts",
            mission.config.base / "workspace"
    };

    for(auto const& dir : required_dirs)
    {
        if(!fs::exists(dir))
        {
            fs::create_directories(dir);
        }
    }

    // Create logger
    Logger logger("Daemon", mission.config.base / "log" / "daemon.log");

    logger.Log("Daemon launched from `%s`", mission.config.binary.c_str());
    logger.Log("Using `%s` as mission base.", mission.config.base.c_str());

    // Register signal handler
    std::signal(SIGTERM, signal_handler);

    /*
     * If the agent returns, then we allow the program to exit.
     * Otherwise, we attempt to restart.
     */
    while(true)
    {
        // Configure agent
        Agent agent(mission);
        IpcServer server(mission, agent);

        try
        {
            // Start IPC server
            logger.Log("Starting server...");
            server.StartServer();

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

            logger.Error("Nysd has encountered the following unrecoverable error: %s", exception.what());
            logger.Error("Restarting in 10s...");

            std::this_thread::sleep_for(std::chrono::seconds(10));
        }
    }
}