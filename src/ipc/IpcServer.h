#ifndef NOT_YOUR_SCHEDULER_IPCSERVER_H
#define NOT_YOUR_SCHEDULER_IPCSERVER_H

#include <stdexcept>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <cstdlib>
#include <cstring>

#define BACKLOG_MAX 10

class IpcServer
{
private:
    sockaddr_un addr;
    int sockfd = -1;

public:
    void StartServer(const char *socket_path)
    {
        // Set the socket address
        memset(&this->addr, 0, sizeof(sockaddr_un));
        this->addr.sun_family = PF_LOCAL;
        this->addr.sun_len = strlen(socket_path) + 1;

        // see `man unix`
        if(this->addr.sun_len > 104)
        {
            throw std::runtime_error("Socket path too long!");
        }

        memcpy(&this->addr.sun_path, socket_path, this->addr.sun_len);

        // Create the socket
        this->sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);
        if(this->sockfd == -1)
        {
            throw std::runtime_error("Failed to create socket for server!");
        }

        // Bind to the socket
        if(bind(this->sockfd, (const sockaddr*)&this->addr, sizeof(sockaddr_un)) != 0)
        {
            throw std::runtime_error("Failed to bind to socket!");
        }

        // Start listening
        if(listen(this->sockfd, BACKLOG_MAX) != 0)
        {
            throw std::runtime_error("Failed to start listening to socket!");
        }
    }
};

#endif //NOT_YOUR_SCHEDULER_IPCSERVER_H
