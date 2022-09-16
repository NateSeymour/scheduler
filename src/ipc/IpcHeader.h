#ifndef NOT_YOUR_SCHEDULER_IPCHEADER_H
#define NOT_YOUR_SCHEDULER_IPCHEADER_H

#include <cstdint>
#include "../message/NysMessage.h"

#define IPC_HEADER_SIZE 12

struct IpcHeader
{
    char magic[6];
    uint8_t version;
    uint16_t echo;
    NysMessageType message_type;
    uint16_t body_length;
};

#endif //NOT_YOUR_SCHEDULER_IPCHEADER_H
