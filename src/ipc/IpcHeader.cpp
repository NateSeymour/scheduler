#include <cstring>
#include <arpa/inet.h>
#include "IpcHeader.h"

IpcHeader IpcHeader::FromBuffer(char buffer[IPC_HEADER_SIZE])
{
    IpcHeader header{};

    // Populate header structure
    memcpy(&header.magic, buffer, 6);
    header.version = *(uint8_t*)(buffer + 6);
    header.echo = ntohs(*(uint16_t*)(buffer + 7));
    header.message_type = *(NysMessageType*)(buffer + 9);
    header.body_length = ntohs(*(uint16_t*)(buffer + 10));

    return header;
}

size_t IpcHeader::WriteToBuffer(char *buffer, size_t buffer_size) const
{
    if(buffer_size < IPC_HEADER_SIZE) return 0;

    memset(buffer, 0, IPC_HEADER_SIZE);

    strncpy(buffer, this->magic, 6);
    *(uint8_t*)(buffer + 6) = this->version;
    *(uint16_t*)(buffer + 7) = htons(this->echo);
    *(uint8_t*)(buffer + 9) = this->message_type;
    *(uint16_t*)(buffer + 10) = htons(this->body_length);

    return IPC_HEADER_SIZE;
}
