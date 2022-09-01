#ifndef NOT_YOUR_SCHEDULER_LAUNCHENVIRONMENT_H
#define NOT_YOUR_SCHEDULER_LAUNCHENVIRONMENT_H

#include <filesystem>

struct LaunchEnvironment {
    std::filesystem::path home_directory;
    std::filesystem::path binary_path;
};

#endif //NOT_YOUR_SCHEDULER_LAUNCHENVIRONMENT_H
