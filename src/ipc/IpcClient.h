#ifndef NOT_YOUR_SCHEDULER_IPCCLIENT_H
#define NOT_YOUR_SCHEDULER_IPCCLIENT_H

#include <filesystem>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include "IpcHeader.h"
#include "../message/NysMessage.h"

class IpcClient
{
private:
    int sockfd = -1;

public:
    void SendMessage(NysMessage message)
    {
        IpcHeader header {
            .magic = {'N', 'Y', 'S', 'I', 'P', 'C'},
            .version = 1,
            .echo = 0,
            .message_type = message.type,
            .body_length = 0
        };

        // Populate buffer
        char header_buffer[IPC_HEADER_SIZE];
        memset(header_buffer, 0, IPC_HEADER_SIZE);

        strncpy(header_buffer, header.magic, 6);
        *(uint8_t*)(header_buffer + 6) = header.version;
        *(uint16_t*)(header_buffer + 7) = htons(header.echo);
        *(uint8_t*)(header_buffer + 9) = header.message_type;
        *(uint16_t*)(header_buffer + 10) = htons(header.body_length);

        // Send buffer
        write(this->sockfd, header_buffer, IPC_HEADER_SIZE);
    }

    void Connect(const std::filesystem::path& socket_path)
    {
        this->sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);

        struct sockaddr_un addr;
        addr.sun_family = PF_LOCAL;
        addr.sun_len = strlen(socket_path.c_str());
        memcpy(addr.sun_path, socket_path.c_str(), addr.sun_len);

        connect(this->sockfd, (const sockaddr*)&addr, sizeof(addr));
    }

    ~IpcClient()
    {
        if(this->sockfd != -1)
        {
            close(this->sockfd);
        }
    }
};

#endif //NOT_YOUR_SCHEDULER_IPCCLIENT_H
