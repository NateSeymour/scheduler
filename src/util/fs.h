#ifndef NOT_YOUR_SCHEDULER_FS_H
#define NOT_YOUR_SCHEDULER_FS_H

#include <vector>
#include <filesystem>

namespace nys::fs {
    std::filesystem::path resolve_path(const std::vector<std::filesystem::path>& paths);
    std::filesystem::path find_resource(const std::filesystem::path& hint);
}

#endif //NOT_YOUR_SCHEDULER_FS_H
