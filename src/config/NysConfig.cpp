#include <toml++/toml.h>
#include <filesystem>
#include "NysConfig.h"
#include "../util/fs.h"

namespace fs = std::filesystem;

NysConfig NysConfig::FromEnv(int argc, const char **argv)
{
    NysConfig nys_config;

    nys_config.base = fs::path(std::getenv("HOME")) / ".nys"; // Default. Overridden by nys.toml
    nys_config.binary = argv[0];
    nys_config.database_resources = nys::fs::find_resource("share/database");
    nys_config.config_path = nys::fs::find_resource("etc/nys.toml");

    // Process nys_config file
    auto config_file = toml::parse_file(nys_config.config_path.string());

    if(config_file.at_path("mission.base").is_string())
    {
        nys_config.base = config_file.at_path("mission.base").as_string()->get();
    }

    return std::move(nys_config);
}
