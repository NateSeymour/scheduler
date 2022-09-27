#ifndef NOT_YOUR_SCHEDULER_IPCCONNECTION_H
#define NOT_YOUR_SCHEDULER_IPCCONNECTION_H

#include <unistd.h>
#include <filesystem>
#include <future>
#include "../message/NysMessage.h"

#define DEFAULT_IPC_TIMEOUT_MS 3000

enum class IpcConnectionType
{
    REQUEST,
    BROADCAST_RECEIVER
};

class IpcConnection
{
protected:
    int fd = -1;

public:
    IpcConnectionType type = IpcConnectionType::REQUEST;

    int GetFd() const;
    bool HasData() const;

    NysMessage GetMessage(int timeout = -1) const;
    void SendMessage(const NysMessage& message) const;

    std::future<NysMessage> Response(int timeout);
    std::future<NysMessage> Response();
    std::future<NysMessage> Request(const NysMessage& message);

    IpcConnection() = delete;
    explicit IpcConnection(int fd) : fd(fd) {}
    explicit IpcConnection(const std::filesystem::path& socket_path);

    // Delete copy and assignment constructors
    IpcConnection(const IpcConnection&) = delete;
    IpcConnection& operator=(const IpcConnection&) = delete;

    // Close connection on delete
    ~IpcConnection();
};


#endif //NOT_YOUR_SCHEDULER_IPCCONNECTION_H
