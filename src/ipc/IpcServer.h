#ifndef NOT_YOUR_SCHEDULER_IPCSERVER_H
#define NOT_YOUR_SCHEDULER_IPCSERVER_H

#include <stdexcept>
#include <filesystem>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <memory>
#include <vector>
#include <arpa/inet.h>
#include "../config/AgentMission.h"
#include "../Logger.h"
#include "IpcHeader.h"

#define BACKLOG_MAX 10

enum Identity : uint64_t
{
    SERVER
};

enum UserServerEvents : uint64_t
{
    SERVER_SHUTDOWN
};

class IpcServer
{
private:
    sockaddr_un addr;
    int sockfd = -1;
    int kq = -1;
    AgentMission mission;

    std::vector<int> connections;

    std::unique_ptr<std::thread> connection_listener;
    std::unique_ptr<Logger> logger;

    void ProcessClientMessage(int fd)
    {
        /*
         * We have to create a header_buffer to read into _and_ a packet object, instead of
         * just reading directly into the packet object, because modern compilers will
         * pad structures for optimized memory access/alignment.
         */
        IpcHeader header{};

        char header_buffer[IPC_HEADER_SIZE];
        memset(header_buffer, 0, IPC_HEADER_SIZE);

        // Read the client's header
        size_t bytes_in = read(fd, header_buffer, IPC_HEADER_SIZE);
        if(bytes_in < 0)
        {
            this->logger->Error("The following error caused a dropped message from (%i): %s.", fd, strerror(errno));
            return;
        }

        if(bytes_in < IPC_HEADER_SIZE)
        {
            this->logger->Error("Received invalid message from (%i): Incomplete header.", fd);
            return;
        }

        // Populate header structure
        memcpy(&header.magic, header_buffer, 6);
        header.version = *(uint8_t*)(header_buffer + 6);
        header.echo = ntohs(*(uint16_t*)(header_buffer + 7));
        header.message_type = *(NysMessageType*)(header_buffer + 9);
        header.body_length = ntohs(*(uint16_t*)(header_buffer + 10));

        // Sanity checks
        if(strncmp(header.magic, "NYSIPC", 6) != 0)
        {
            this->logger->Error("Received invalid message from (%i): Missing magic.", fd);
            return;
        }

        // Sanity checks
        if(header.version != 1)
        {
            this->logger->Error("Received invalid message from (%i): Unsupported protocol version %i.", fd, header.version);
            return;
        }

        // Process message
        switch(header.message_type)
        {
            /*
             * Relay message
             */
            case MESSAGE_SHUTDOWN:
            case MESSAGE_RELOAD:
            {
                this->mission.broadcaster->Broadcast({
                    .type = header.message_type
                });
                break;
            }

            /*
             * Ignore message
             */
            case MESSAGE_NONE:
            case MESSAGE_NEW_CLIENT:
                break;
        }
    }

    void CloseClientConnection(int fd)
    {
        /*
         * We don't need to remove the connection from the kqueue explicitly, because
         * filters are automatically removed upon the last `close` of their associated
         * file descriptor. See `man kqueue`.
         */
        close(fd);

        auto it = std::remove_if(this->connections.begin(), this->connections.end(), [fd](int conn) {
            return conn == fd;
        });
        this->connections.erase(it, this->connections.end());

        this->logger->Log("Client disconnected (%i). %i active clients.", fd, this->connections.size());
    }

    void AcceptClient()
    {
        int new_connection = accept(this->sockfd, nullptr, nullptr);

        if(new_connection == -1)
        {
            this->logger->Error("WARN Problem accepting new client connection.");
            return;
        }

        // Add new client to kqueue events
        struct kevent64_s conn_event{};
        EV_SET64(&conn_event, new_connection, EVFILT_READ, EV_ADD | EV_EOF | EV_CLEAR, 0, 0, 0, 0, 0);
        kevent64(this->kq, &conn_event, 1, nullptr, 0, 0, nullptr);

        this->connections.push_back(new_connection);

        this->mission.broadcaster->Broadcast({
            .type = MESSAGE_NEW_CLIENT
        });
        this->logger->Log("New connection received (%i). %i active clients.", new_connection, this->connections.size());
    }

    int ProcessEvent(struct kevent64_s event)
    {
        switch(event.filter)
        {
            case EVFILT_USER:
            {
                if(event.udata == UserServerEvents::SERVER_SHUTDOWN) return 1;
                break;
            }

            case EVFILT_READ:
            {
                // There is a new connection to be accepted
                if(event.ident == this->sockfd)
                {
                    this->AcceptClient();
                }
                else // There is data to be read from one of the clients
                {
                    int client_fd = (int)event.ident;

                    // Client has disconnected
                    if(event.flags & EV_EOF)
                    {
                        this->CloseClientConnection(client_fd);
                        break;
                    }

                    // Client has sent data
                    this->ProcessClientMessage(client_fd);
                }
                break;
            }
        }

        return 0;
    }

    void ConnectionListener()
    {
        /*
         * MacOS specific, probably needs to be rewritten with `epoll` for Linux.
         * The problem here is that we need to wait for incoming connections to become
         * available so that we can accept them, but we also need to know when the server
         * is being shutdown so that we can gracefully exit (and restart without unexpected
         * side effects).
         *
         * I see three ways to do this
         * 1. Poll not just for incoming data, but for kernel events as well with
         *      `epoll`/`kqueue`. The downside is less portability and more code to maintain.
         * 2. Set a timeout that will restart or fail depending on the server's shutdown
         *      condition. This would be ugly and use more resources.
         * 3. Use `libevent`
         */

        // Register READ event
        const int change_count = 1;

        struct kevent64_s change_list[change_count];
        memset(change_list, 0, sizeof(struct kevent64_s) * change_count);

        EV_SET64(&change_list[0], this->sockfd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, 0, 0, 0);

        kevent64(this->kq, change_list, change_count, nullptr, 0, 0, nullptr);

        // Handle incoming kernel events
        const int event_count = 5;
        struct kevent64_s event_list[event_count];
        while(true)
        {
            int nevents = kevent64(this->kq, nullptr, 0, event_list, event_count, 0, nullptr);

            if(nevents == -1) throw std::runtime_error("Issue waiting for connections!");

            for(int i = 0; i < nevents; i++)
            {
                if(this->ProcessEvent(event_list[i]) != 0)
                {
                    return;
                }
            }
        }
    }

public:
    void StartServer(const AgentMission& server_mission)
    {
        this->mission = server_mission;

        std::filesystem::path socket_path = this->mission.base / "nys.sock";

        this->logger = std::make_unique<Logger>("IpcServer", this->mission.base / "log" / "ipc_server.log");

        this->logger->Log("Initializing IPC server...");

        // Set the socket address
        memset(&this->addr, 0, sizeof(sockaddr_un));
        this->addr.sun_family = PF_LOCAL;
        this->addr.sun_len = strlen(socket_path.c_str()) + 1;

        // see `man unix`
        if(this->addr.sun_len > 104)
        {
            throw std::runtime_error("Socket path too long!");
        }

        memcpy(&this->addr.sun_path, socket_path.c_str(), this->addr.sun_len);

        // Create the socket
        this->sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);
        if(this->sockfd == -1)
        {
            throw std::runtime_error("Failed to create socket for server!");
        }

        // Make the socket non-blocking (we use kqueue to detect new clients)
        fcntl(this->sockfd, F_SETFL, O_NONBLOCK);

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

        // Create kqueue for server events
        this->kq = kqueue();

        if(this->kq == -1)
        {
            throw std::runtime_error("Issue allocating kernel queue!");
        }

        // Register user events
        const int user_event_count = 1;

        struct kevent64_s user_events[user_event_count];
        EV_SET64(&user_events[0], Identity::SERVER, EVFILT_USER, EV_ADD | EV_ENABLE, NULL, NULL, NULL, NULL, NULL);
        kevent64(this->kq, user_events, user_event_count, nullptr, 0, 0, nullptr);

        // Start accepting connections
        this->logger->Log("Listening for connections...");

        this->connection_listener = std::make_unique<std::thread>(&IpcServer::ConnectionListener, this);
    }

    ~IpcServer()
    {
        if(this->sockfd != -1)
        {
            // Notify server to shut down
            struct kevent64_s shutdown_event{};
            EV_SET64(&shutdown_event, Identity::SERVER, EVFILT_USER, 0, NOTE_TRIGGER, NULL, UserServerEvents::SERVER_SHUTDOWN, NULL, NULL);
            kevent64(this->kq, &shutdown_event, 1, nullptr, 0, 0, nullptr);

            this->connection_listener->join();
            close(this->kq);

            // Close all server connections
            for(auto const& conn : this->connections)
            {
                close(conn);
            }

            // Close server socket and unlink socket from filesystem
            close(this->sockfd);
            unlink(this->addr.sun_path);
        }
    }
};

#endif //NOT_YOUR_SCHEDULER_IPCSERVER_H
