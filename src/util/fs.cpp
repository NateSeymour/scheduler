#include "fs.h"

using namespace nys::fs;

std::filesystem::path nys::fs::resolve_path(const std::vector<std::filesystem::path>& paths)
{
    for(auto const& path : paths)
    {
        if(std::filesystem::exists(path)) return path;
    }

    throw std::runtime_error("Unable to find required resource");
}

std::filesystem::path nys::fs::find_resource(const std::filesystem::path & hint)
{
    return resolve_path({
            "/opt/homebrew" / hint,
            "/usr/local" / hint,
            ".." / hint,
            "." / hint,
            ".." / hint.filename(),
            "." / hint.filename()
    });
}