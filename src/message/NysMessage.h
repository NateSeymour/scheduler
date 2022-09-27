#ifndef NOT_YOUR_SCHEDULER_NYSMESSAGE_H
#define NOT_YOUR_SCHEDULER_NYSMESSAGE_H

#include <nlohmann/json.hpp>

enum NysMessageType : uint8_t
{
    MESSAGE_NONE = 0,
    MESSAGE_SHUTDOWN = 1,
    MESSAGE_RELOAD = 2,
    MESSAGE_NEW_CLIENT = 3,
    MESSAGE_TIMEOUT = 4,
    MESSAGE_OK = 5
};

struct NysMessage
{
    NysMessageType type;
    nlohmann::json value;
};


#endif //NOT_YOUR_SCHEDULER_NYSMESSAGE_H
