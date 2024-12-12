#include <freertos/FreeRTOS.h> // Mandatory first include

#include <esp_log.h>
#include <esp_netif.h>
#include <netdb.h>
#include <sys/poll.h>

#include <soccerbot/log.h>
#include <soccerbot/net.h>
#include <soccerbot/util.h>

#define TAG "netthread"

namespace soccerbot {

void startTcpServer();
void acceptTcpServer(int acceptSock);

TcpClient* clients[MAX_TCP_SOCKETS] = {};
int clientsCount = 0;

// Accept sockets first, then recieve sockets since there are more of them
pollfd pollDescriptors[MAX_TCP_SOCKETS] = {};

void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("\n");
}

void invalidateClient(TcpClient* client) {
    bool remove = client->invalidate();

    if (remove) {
        bool found = false;
        for (int i = 0; i < clientsCount; i++) {
            if (!found && clients[i] == client) {
                found = true;
            }
            
            if (found) {
                clients[i] = clients[i + 1];
            }
        }

        clientsCount--;
    }
}

// Puts TCP socket output into strings, re-establishes closed/crashed sockets
void runSockets()
{
    int rc = poll(pollDescriptors, clientsCount, 5000);
    if (rc < 0) {
        ESP_LOGI("", "Poll failed!");
        for (int i = 0; i < clientsCount; i++) {
            invalidateClient(clients[i]);
        }
    }
    else if (rc == 0) {
        // Poll just timed out
        ESP_LOGI("", "Poll timed out!");
        return;
    } else if (rc > 0) {
        // Something happened to a listening descriptor
        for (int i = 0; i < clientsCount; i++) {
            int revents = pollDescriptors[i].revents;

            // This is designed so that remaining data is read first, then an invalid state is handled
            if (revents & POLLIN) {
                clients[i]->recv();
            } 
            if (revents & POLLNVAL) {
                // Was never valid
                ESP_LOGI("", "Invalidating for netThread 1");
                invalidateClient(clients[i]);
            }
            if (revents & (POLLHUP | POLLERR)) {
                // Got closed
                ESP_LOGI("", "Invalidating for netThread 2");
                invalidateClient(clients[i]);
            }
        }
    }
}

void acceptThread(void*)
{
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    addrinfo* serverSocketInfo = nullptr;
    CHECK(getaddrinfo(NULL, "80", &hints, &serverSocketInfo) == 0);

    int sock = -1;

    for (addrinfo* i = serverSocketInfo; i != nullptr; i = i->ai_next) {
        sock = socket(i->ai_family, i->ai_socktype, i->ai_protocol);
        if (sock < 0) {
            ESP_LOGE("rnet", "socket() error, nonfatal: %s", strerror(errno));
            continue;
        }

        int on = 1;
        CHECK(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == 0);;

        CHECK(bind(sock, i->ai_addr, i->ai_addrlen) == 0);

        break;
    }

    freeaddrinfo(serverSocketInfo);

    CHECK(listen(sock, 5) == 0);

    while (true)
    {
        acceptTcpServer(sock);
    }
}

void startNetThread()
{   
    for (int i = 0; i < MAX_TCP_SOCKETS; i++) {
        pollDescriptors[i].fd = -1;
    }

    // Now done by robotThread
    //xTaskCreate(netThread, "net", TaskStackSize::LARGE, nullptr, TaskPriority::NET, nullptr);

    xTaskCreate(acceptThread, "accept", TaskStackSize::LARGE, nullptr, TaskPriority::NET, nullptr);

    ESP_LOGI("", "Started net thread");
}

TcpClient* addTcpClient(uint32_t targetIp, uint16_t port)
{
    ESP_LOGI("", "Adding client to slot %d", clientsCount);

    CHECK(clientsCount < MAX_TCP_SOCKETS);
    TcpClient* c = new TcpClient(targetIp, port);
    clients[clientsCount] = c;
    c->connect();

    pollDescriptors[clientsCount] = {
        .fd = c->sock,
        .events = POLLERR | POLLHUP | POLLIN
    };

    return clients[clientsCount++];
}

// Add a client to poll from an already created socket
TcpClient* addTcpClientFromSock(int fd, sockaddr_in* addr)
{
    ESP_LOGI("", "Adding client to slot %d", clientsCount);

    CHECK(clientsCount < MAX_TCP_SOCKETS);
    TcpClient* c = new TcpClient(fd, addr);
    clients[clientsCount] = c;

    pollDescriptors[clientsCount] = {
        .fd = c->sock,
        .events = POLLERR | POLLHUP | POLLIN
    };

    clientsCount += 1;
    return clients[clientsCount - 1];
}

void acceptTcpServer(int acceptSock)
{
    sockaddr sourceAddr;
    socklen_t addrLen = sizeof(sourceAddr);
    ESP_LOGI("", "Socket is ready for accept");
    int sock = accept(acceptSock, (struct sockaddr*)&sourceAddr, &addrLen);
    if (sock < 0) {
        ESP_LOGI("", "Unable to accept connection: errno %d", errno);
        return;
    }
    ESP_LOGI("", "postAccept 2");

    int yes = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int));
    
    int keepIdle = 5; // Time between keepalive probe packets when there is no peer activity
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));

    int keepInterval = 5;
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));

    int keepCount = 3; // Number of times to retry keepalive
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));

    // Disable Congestion Control / Nagle's Algorithm to prevent waiting for large packets to send
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));

    int flags = fcntl(sock, F_GETFL, NULL);
    CHECK(flags >= 0);
    flags |= O_NONBLOCK;
    CHECK(fcntl(sock, F_SETFL, flags));

    char addrStr[16];
    inet_ntoa_r(((sockaddr_in *)&sourceAddr)->sin_addr, addrStr, sizeof(addrStr) - 1);
    ESP_LOGI("", "Opened socket to %s", addrStr);

    TcpClient* client = addTcpClientFromSock(sock, (sockaddr_in*)&sourceAddr);
    client->waitToConnect();

    ESP_LOGI("", "Sending hello");
    client->sendHello();
}

}; // namespace soccerbot