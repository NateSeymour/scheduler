#include <sys/socket.h>
#include <sys/un.h>
#include <cstring>
#include <iostream>
#include <thread>
#include <chrono>

int main()
{
    // Connect to server
    int sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);

    const char *path = "/Users/nathan/.nys/nys.sock";

    struct sockaddr_un addr{};
    addr.sun_family = PF_LOCAL;
    addr.sun_len = strlen(path) + 1;

    memcpy(&addr.sun_path, path, addr.sun_len);

    std::cout << connect(sockfd, (const sockaddr*)&addr, sizeof(addr)) << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}