#ifndef NOT_YOUR_SCHEDULER_IPCCLIENT_H
#define NOT_YOUR_SCHEDULER_IPCCLIENT_H

#include <filesystem>
#include <cstring>
#include <future>
#include <unistd.h>
#include <thread>
#include <memory>
#include <poll.h>
#include <arpa/inet.h>
#include "IpcConnection.h"
#include "../config/NysConfig.h"
#include "../Logger.h"
#include "IpcHeader.h"
#include "../message/NysMessage.h"

class IpcClient
{
private:
    NysConfig config;
    std::unique_ptr<Logger> logger;

public:
    IpcConnection GetConnection() const
    {
        return IpcConnection(this->config.base / "nys.sock");
    }

    explicit IpcClient(NysConfig config) : config(std::move(config))
    {
        this->logger = std::make_unique<Logger>("IpcClient", this->config.base / "log" / "ipc_client.log");
    }
};

#endif //NOT_YOUR_SCHEDULER_IPCCLIENT_H
