#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <nlohmann/json.hpp>
#include <poll.h>
#include "IpcConnection.h"
#include "IpcHeader.h"
#include "../format.h"

int IpcConnection::GetFd() const
{
    return this->fd;
}

IpcConnection::~IpcConnection()
{
     if(this->fd != -1)
    {
        close(this->fd);
    }
}

IpcConnection::IpcConnection(const std::filesystem::path& socket_path)
{
    this->fd = socket(PF_LOCAL, SOCK_STREAM, 0);

    if(this->fd == -1)
    {
        throw std::runtime_error("Failed to create socket!");
    }

    struct sockaddr_un addr{};
    addr.sun_family = PF_LOCAL;
    addr.sun_len = strlen(socket_path.c_str());
    memcpy(addr.sun_path, socket_path.c_str(), addr.sun_len);

    if(connect(this->fd, (const sockaddr*)&addr, sizeof(addr)) != 0)
    {
        throw std::runtime_error("Failed to connect to server!");
    }
}

bool IpcConnection::HasData() const
{
    return false;
}

NysMessage IpcConnection::GetMessage(int timeout) const
{
    /*
     * We use `poll` to be able to implement a response timeout.
     * Wait for incoming data before starting read.
     */
    struct pollfd fds = {
            .fd = this->fd,
            .events = POLLIN,
            .revents = 0
    };

    int nfds = poll(&fds, 1, timeout);

    if(nfds == 0)
    {
        throw std::runtime_error("Reached timout on expected message!");
    }

    // Parse message
    NysMessage message;

    char header_buffer[IPC_HEADER_SIZE];
    memset(header_buffer, 0, IPC_HEADER_SIZE);

    // Read the client's header
    size_t header_bytes_in = read(this->fd, header_buffer, IPC_HEADER_SIZE);
    if(header_bytes_in < 0)
    {
        throw std::runtime_error(nys::format("The following error caused a dropped message on (%i): %s.", this->fd, strerror(errno)));
    }

    if(header_bytes_in < IPC_HEADER_SIZE)
    {
        throw std::runtime_error(nys::format("Received invalid message on (%i): Incomplete header.", this->fd));
    }

    // Parse header
    IpcHeader header = IpcHeader::FromBuffer(header_buffer);

    // Sanity checks
    if(strncmp(header.magic, "NYSIPC", 6) != 0)
    {
        throw std::runtime_error(nys::format("Received invalid message on (%i): Missing magic.", this->fd));
    }

    // Sanity checks
    if(header.version != 1)
    {
        throw std::runtime_error(nys::format("Received invalid message on (%i): Unsupported protocol version %i.", this->fd, header.version));
    }

    // Set message type
    message.type = header.message_type;

    // Read in message body
    if(header.body_length > 0)
    {
        auto message_body = (char*)calloc(1, header.body_length);
        size_t body_bytes_in = read(this->fd, message_body, header.body_length);

        message.value = nlohmann::json::parse(message_body);

        free(message_body);
    }
    else
    {
        message.value = nlohmann::json::object();
    }

    return message;
}

void IpcConnection::SendMessage(const NysMessage& message) const
{
    std::string body_string = "";
    if(!message.value.empty() && message.value.type() == nlohmann::json::value_t::object)
    {
        body_string = message.value.dump();
    }

    IpcHeader header {
            .magic = {'N', 'Y', 'S', 'I', 'P', 'C'},
            .version = 1,
            .echo = 0,
            .message_type = message.type,
            .body_length = (uint16_t)body_string.size()
    };

    // Populate buffer
    char header_buffer[IPC_HEADER_SIZE];
    header.WriteToBuffer(header_buffer, IPC_HEADER_SIZE);

    // Send header
    write(this->fd, header_buffer, IPC_HEADER_SIZE);

    // Send body
    write(this->fd, body_string.c_str(), header.body_length);
}

std::future<NysMessage> IpcConnection::Response(int timeout)
{
    // Spawn thread that will listen for response
    std::promise<NysMessage> response_promise;
    std::future<NysMessage> response_future = response_promise.get_future();

    std::thread response_listener([this, timeout, promise = std::move(response_promise)] () mutable {
        promise.set_value(this->GetMessage(timeout));
    });
    response_listener.detach();

    return response_future;
}

std::future<NysMessage> IpcConnection::Response()
{
    // 10 second timeout
    return this->Response(10000);
}

std::future<NysMessage> IpcConnection::Request(const NysMessage& message)
{
    this->SendMessage(message);
    return this->Response();
}
