#ifndef NOT_YOUR_SCHEDULER_NYSMESSAGE_H
#define NOT_YOUR_SCHEDULER_NYSMESSAGE_H

enum NysMessageType
{
    MESSAGE_NONE,
    MESSAGE_SHUTDOWN
};

struct NysMessage
{
    NysMessageType type;

    union
    {
        char shutdown_reason[100];
    };
};


#endif //NOT_YOUR_SCHEDULER_NYSMESSAGE_H
