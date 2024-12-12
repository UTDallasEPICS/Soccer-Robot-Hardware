#ifndef SOCCERBOT_NET_H
#define SOCCERBOT_NET_H

#include <errno.h>
#include <lwip/sockets.h>
#include <sys/socket.h>
#include <esp_mac.h>

#include <freertos/event_groups.h>
#include <freertos/stream_buffer.h>

#include <string_view>

#include <soccerbot/log.h>
#include <soccerbot/unit.h>
#include <string>

namespace soccerbot {
class TcpClient {
public:
    constexpr static const char* TAG = "net";

    constexpr static int TCP_BUF_SIZE = 1024;
    constexpr static int TCP_MSG_SIZE_MAX = 1024;
    constexpr static std::pair<TickType_t, TickType_t> TCP_RETRY_FREQUENCY = { 3_s, 10_s };

    constexpr static int STATUS_OPEN = BIT1; // Normal
    constexpr static int STATUS_CONNECTING = BIT2; // Normal

    sockaddr_in destAddr = {};
    int sock = -1;
    EventGroupHandle_t status;
    TickType_t lastRetry = 0;
    bool reconnectOnFail = false;

    // Recieve target
    char rxBuf[TCP_BUF_SIZE] = {};
    char* rxCursor = rxBuf;

    // Create a TCP connection by connecting to a remote socket
    TcpClient(uint32_t targetIp, uint16_t port)
    {
        ESP_LOGI(TAG, "Construct TcpClient with ip, port %d", (int)port);

        makeBufs();

        destAddr.sin_family = AF_INET;
        destAddr.sin_addr.s_addr = targetIp;
        destAddr.sin_port = htons(port);

        reconnectOnFail = true;
    }

    // Create a TCP connection by connecting to a remote socket
    TcpClient(const char* targetIp, uint16_t port)
    {
        ESP_LOGI(TAG, "Construct TcpClient with ip, port %d", (int)port);

        makeBufs();

        destAddr.sin_family = AF_INET;
        inet_pton(AF_INET, targetIp, &destAddr.sin_addr);
        destAddr.sin_port = htons(port);

        reconnectOnFail = true;
    }

    // Create from a socket that already exists
    TcpClient(int fd, sockaddr_in* addr)
    {
        ESP_LOGI(TAG, "Construct TcpClient directly with FD %d", fd);
        sock = fd;
        memcpy(&this->destAddr, &addr, sizeof(addr));

        makeBufs();

        reconnectOnFail = false;

        xEventGroupSetBits(status, STATUS_OPEN);
    }

    void makeBufs()
    {
        status = xEventGroupCreate();
    }

    // Acts on a socket that is no longer usable
    // Returns whether to destroy the object
    bool invalidate()
    {
        ESP_LOGI(TAG, "Invalidating socket");

        int prevSock = sock;

        if (sock != -1) {
            ::close(sock);
            sock = -1;
        }
        
        xEventGroupClearBits(status, STATUS_OPEN);

        if (reconnectOnFail) 
        {
            ESP_LOGI(TAG, "Reconnecting due to reconnectOnFail with FD %d", prevSock);
            connect();
            return false;
        }
        else
        {
            return true;
        }
    }

    void connect()
    {
        if (sock != -1) {
            ESP_LOGE(TAG, "Tried to connect on full socket");
            // Already connected
            return;
        }

        xEventGroupSetBits(status, STATUS_CONNECTING);

        ESP_LOGI(TAG, "Start connect");

        // Backoff
        TickType_t now = xTaskGetTickCount();
        if (now - lastRetry < TCP_RETRY_FREQUENCY.first) {
            return;
        } else {
            lastRetry = now;
        }

        ESP_LOGI(TAG, "Start connect 2");

        sock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock < 0) {
            ESP_LOGE(TAG, "Socket unable to connect: %s", strerror(errno));
            invalidate();
            return;
        }
        // CHECK(::ioctl(sock, O_NONBLOCK, 0) >= 0);
        // CHECK(esp_tls_set_socket_non_blocking(sock, true));

        ESP_LOGI(TAG, "Socket created, connecting");

        int err = ::connect(sock, (sockaddr*)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: %s", strerror(errno));
            invalidate();
            return;
        }
        ESP_LOGI(TAG, "Successfully connected");

        int flags = fcntl(sock, F_GETFL, NULL);
        CHECK(flags >= 0);
        flags |= O_NONBLOCK;
        CHECK(fcntl(sock, F_SETFL, flags));

        xEventGroupClearBits(status, STATUS_CONNECTING);
        xEventGroupSetBits(status, STATUS_OPEN);
    }

    bool isOpen()
    {
        if (sock < 0) {
            ESP_LOGE(TAG, "Invalidating for isOpen 1");
            invalidate();
            return false;
        }

        /*int error = 0;
        socklen_t errorsz = sizeof(error);
        int ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &errorsz);

        if (ret != 0 || error != 0) {
            ESP_LOGI(TAG, "Invalidating for isOpen 2");
            invalidate();
            return false;
        }*/

        return true;
    }

    void waitToConnect()
    {
        xEventGroupWaitBits(status, STATUS_OPEN, pdFALSE, pdFALSE, portMAX_DELAY);
    }

    void send(std::string_view msg)
    {
        int err = ::send(sock, msg.data(), msg.size(), 0);
        if (err < 0) {
            ESP_LOGE(TAG, "Error occurred during sending: %s", strerror(errno));
        }
    }

    int rxBufSpace() 
    {
        ESP_LOGI(TAG, "rxBufSpace %d", (rxBuf + sizeof(rxBuf) - 1) - rxCursor);
        return (rxBuf + sizeof(rxBuf) - 1) - rxCursor;
    }

    void recv()
    {
        if (!isOpen()) {
            ESP_LOGE(TAG, "Socket was not open for recv!");
            connect();
            return;
        }

        errno = 0;
        int len = ::recv(sock, rxCursor, rxBufSpace(), 0);

        if (len < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN || errno == EINPROGRESS) {
                // Just skip it for now
                ESP_LOGE(TAG, "recv 2 failed: %s", strerror(errno));
            } else {
                // Error occurred during receiving, or socket closed
                ESP_LOGE(TAG, "recv failed: %s", strerror(errno));
                invalidate();
            }
        } else {
            rxCursor += len;
            CHECK(rxCursor < (rxBuf + sizeof(rxBuf)));

            // Data received
            ESP_LOGI(TAG, "Received %d bytes", len);

            // Log the content. It's not null terminated, so this works
            ESP_LOGI(TAG, "Received content: %s", rxBuf);
        }
    }

    char* packetEnd() {
        for (char* i = rxBuf; i < rxCursor; i++) {
            if (*i == ';') {
                return i;
            }
        }

        return rxBuf;
    }

    bool hasPacket() {
        return packetEnd() != rxBuf;
    }

    std::string_view readPacket() {
        for (char* i = rxBuf; i < rxCursor; i++) {
            if (*i == ';') {
                return std::string_view(rxBuf, i);
            }
        }

        FAIL();
    }

    // Copy range from end of packet to read cursor to start
    void clearPacket() {
        char* nextPacket = packetEnd() + 1;
        memmove(rxBuf, nextPacket, rxCursor - nextPacket);
        rxCursor = rxBuf + (rxCursor - nextPacket);
    }

    void sendHello()
    {
        char helloPacket[80];
        uint8_t mac[6];
        CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
        int len = snprintf(helloPacket, sizeof(helloPacket), R"({"type":"CLIENT_HELLO","macAddress":"%02X:%02X:%02X:%02X:%02X:%02X"};)", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        send(std::string_view(helloPacket, len));
    }

    ~TcpClient()
    {
        ESP_LOGI(TAG, "Removing TcpClient");

        // todo: this
        shutdown(sock, 0);
        close(sock);
    }
};

void startNetThread();

void runSockets();

TcpClient* addTcpClient(uint32_t targetIp, uint16_t port);

constexpr int MAX_TCP_SOCKETS = 10;

extern TcpClient* clients[MAX_TCP_SOCKETS];
extern int clientsCount;
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_NET_H