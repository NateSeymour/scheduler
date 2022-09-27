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

    static IpcHeader FromBuffer(char buffer[IPC_HEADER_SIZE]);
    size_t WriteToBuffer(char* buffer, size_t buffer_size) const;
};

#endif //NOT_YOUR_SCHEDULER_IPCHEADER_H
