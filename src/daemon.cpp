#include <exception>
#include <iostream>
#include <cstdlib>
#include "AgentConductor.h"
#include "LaunchEnvironment.h"

int main(int argc, const char **argv)
{
    LaunchEnvironment launch_environment;
    launch_environment.home_directory = std::getenv("HOME");
    launch_environment.binary_path = argv[0];

    AgentConductor conductor(launch_environment);
    conductor.Setup();

    try {
        conductor.NewAgent().Run();
    } catch(std::exception& e) {
        std::cerr << "Agent has encountered an exception:\n" << std::endl;
        std::cerr << e.what() << std::endl;
    }
}