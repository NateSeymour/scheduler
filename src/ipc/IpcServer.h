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
#include "../AgentMission.h"
#include "../Logger.h"

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

    std::unique_ptr<std::thread> connection_listener;
    std::unique_ptr<Logger> logger;

public:
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
                switch(event_list[i].filter)
                {
                    case EVFILT_USER:
                    {
                        if(event_list[i].udata == UserServerEvents::SERVER_SHUTDOWN) return;
                        break;
                    }

                    // TODO: handle incoming connections
                    case EVFILT_READ:
                    {
                        std::cout << "Handling accept" << std::endl;
                        struct sockaddr_un conn_addr{};
                        socklen_t len = sizeof(conn_addr);

                        int new_connection = accept(this->sockfd, (sockaddr*)&conn_addr, &len);

                        std::cout << new_connection << std::endl;

                        close(new_connection);

                        break;
                    }
                }
            }
        }
    }

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

        // Make the socket non-blocking
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
            // Notify threads to shut down
            struct kevent64_s shutdown_event{};
            EV_SET64(&shutdown_event, Identity::SERVER, EVFILT_USER, 0, NOTE_TRIGGER, NULL, UserServerEvents::SERVER_SHUTDOWN, NULL, NULL);
            kevent64(this->kq, &shutdown_event, 1, nullptr, 0, 0, nullptr);

            // Close handles and join threads
            close(this->sockfd);
            unlink(this->addr.sun_path);
            this->connection_listener->join();
            close(this->kq);
        }
    }
};

#endif //NOT_YOUR_SCHEDULER_IPCSERVER_H
