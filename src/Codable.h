#ifndef NOT_YOUR_SCHEDULER_CODABLE_H
#define NOT_YOUR_SCHEDULER_CODABLE_H

#include <nlohmann/json.hpp>

struct Encodable
{
    virtual nlohmann::json ToJson() = 0;
};

#endif //NOT_YOUR_SCHEDULER_CODABLE_H
