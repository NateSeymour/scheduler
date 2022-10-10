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
#include "IpcConnection.h"
#include "../Agent.h"

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
    const AgentMission& mission;
    const Agent& agent;

    std::vector<std::unique_ptr<IpcConnection>> connections;

    std::unique_ptr<std::thread> connection_listener;
    std::unique_ptr<Logger> logger;

    void HandleRequest(IpcConnection* fd) const
    {
        NysMessage message = fd->GetMessage();

        // Process message
        switch(message.type)
        {
            /*
             * Relay message
             */
            case MESSAGE_SHUTDOWN:
            case MESSAGE_RELOAD:
            {
                this->mission.broadcaster->Broadcast(message);
                break;
            }

            /*
             * Handle message
             */
            case MESSAGE_DESCRIBE_STATE:
            {
                nlohmann::json response_body = nlohmann::json::object();
                response_body["units"] = nlohmann::json::array();

                for(auto const& unit : this->agent.Units())
                {
                    response_body["units"].push_back(unit->ToJson());
                }

                fd->SendMessage({
                    .type = MESSAGE_DESCRIBE_STATE,
                    .value = response_body
                });
                return;
            }

            /*
             * Ignore message
             */
            case MESSAGE_TIMEOUT:
            case MESSAGE_OK:
            case MESSAGE_NONE:
            case MESSAGE_NEW_CLIENT:
                break;
        }

        fd->SendMessage({
                .type = MESSAGE_OK
        });
    }

    void CloseClientConnection(IpcConnection* client)
    {
        /*
         * We don't need to close the connection explicitly, because the connection
         * deconstructor will do this for us.
         *
         * We don't need to remove the connection from the kqueue explicitly, because
         * filters are automatically removed upon the last `close` of their associated
         * file descriptor. See `man kqueue`.
         */

        this->logger->Log("Client disconnecting (%i). %i active clients.", client->GetFd(), this->connections.size() - 1);

        auto it = std::remove_if(this->connections.begin(), this->connections.end(), [client](auto const& conn) {
            return conn->GetFd() == client->GetFd();
        });
        this->connections.erase(it, this->connections.end());
    }

    void AcceptClient()
    {
        auto new_connection = std::make_unique<IpcConnection>(accept(this->sockfd, nullptr, nullptr));

        if(new_connection->GetFd() == -1)
        {
            this->logger->Error("WARN Problem accepting new client connection.");
            return;
        }

        /*
         * Add new client to kqueue events
         * We supply the connection pointer (raw) to the `udata` argument
         */
        struct kevent64_s conn_event{};
        EV_SET64(&conn_event, new_connection->GetFd(), EVFILT_READ, EV_ADD | EV_EOF | EV_CLEAR, 0, 0, (uint64_t)new_connection.get(), 0, 0);
        kevent64(this->kq, &conn_event, 1, nullptr, 0, 0, nullptr);

        this->logger->Log("New connection received (%i). %i active clients.", new_connection->GetFd(), this->connections.size());
        this->connections.push_back(std::move(new_connection));
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
                    auto client = (IpcConnection*)event.udata;

                    // Client has sent data
                    if(event.data > 0)
                    {
                        this->HandleRequest(client);
                    }

                    // Client has disconnected
                    if(event.flags & EV_EOF)
                    {
                        this->CloseClientConnection(client);
                    }
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
    void StartServer()
    {
        this->logger->Log("Starting IPC server...");

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

    IpcServer(const AgentMission& mission, const Agent& agent) : mission(mission), agent(agent)
    {
        std::filesystem::path socket_path = this->mission.config.base / "nys.sock";

        this->logger = std::make_unique<Logger>("IpcServer", this->mission.config.base / "log" / "ipc_server.log");

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

            // Close server socket and unlink socket from filesystem
            close(this->sockfd);
            unlink(this->addr.sun_path);
        }
    }
};

#endif //NOT_YOUR_SCHEDULER_IPCSERVER_H
