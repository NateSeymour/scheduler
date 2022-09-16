#ifndef NOT_YOUR_SCHEDULER_NYSMESSAGE_H
#define NOT_YOUR_SCHEDULER_NYSMESSAGE_H

enum NysMessageType : uint8_t
{
    MESSAGE_NONE = 0,
    MESSAGE_SHUTDOWN = 1,
    MESSAGE_RELOAD = 2,
    MESSAGE_NEW_CLIENT = 3
};

struct NysMessage
{
    NysMessageType type;

    union
    {
        const char *shutdown_reason;
        const char *reload_reason;
    };
};


#endif //NOT_YOUR_SCHEDULER_NYSMESSAGE_H
