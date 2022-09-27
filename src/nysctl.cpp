#include <iostream>
#include <filesystem>
#include <cstring>
#include "config/NysConfig.h"
#include "Logger.h"
#include "ipc/IpcClient.h"

void print_usage()
{
    std::cout << "Usage: nysctl <command> <subcommand>" << std::endl;
    std::cout << "Valid commands: " << std::endl;
    std::cout << "\treload   - Reloads `nysd` to reflect changes to units" << std::endl;
    std::cout << "\tlist     - List all units and their current states" << std::endl;
    std::cout << "\tversion  - Print version information" << std::endl;
    std::cout << "\thelp     - Print this help text, or help information about <subcommand>" << std::endl;
}

int main(int argc, const char **argv)
{
    if(argc != 2)
    {
        print_usage();
        return 1;
    }

    // Get config
    NysConfig config = NysConfig::FromEnv(argc, argv);

    // Create required paths
    std::filesystem::create_directories(config.base / "log");

    // Create logger
    Logger logger("NysCtl", config.base / "log" / "nysctl.log");

    IpcClient client(config);

    // Process command
    const char *command = argv[1];

    if(strcmp(command, "reload") == 0)
    {
        logger.Log("Reloading `nysd`...");

        auto response = client.GetConnection().Request({
            .type = MESSAGE_RELOAD,
            .value = {
                    {"reason", "User requested reload via nysctl"}
            }
        }).get();

        return 0;
    }

    logger.Error("Unrecognized command: `%s`", command);
    return 1;
}