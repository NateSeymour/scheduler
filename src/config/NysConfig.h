#ifndef NOT_YOUR_SCHEDULER_NYSCONFIG_H
#define NOT_YOUR_SCHEDULER_NYSCONFIG_H

#include <filesystem>

struct NysConfig
{
    std::filesystem::path base;
    std::filesystem::path binary;
    std::filesystem::path database_resources;
    std::filesystem::path config_path;

    static NysConfig FromEnv(int argc, const char **argv);
};

#endif //NOT_YOUR_SCHEDULER_NYSCONFIG_H
