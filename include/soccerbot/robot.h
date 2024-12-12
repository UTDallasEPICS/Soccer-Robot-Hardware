#ifndef SOCCERBOT_ROBOT_H
#define SOCCERBOT_ROBOT_H

#include <array>
#include <optional>

#include <esp_flash_partitions.h>
#include <esp_ota_ops.h>
#include <esp_sleep.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/ip4_addr.h>

#include <soccerbot/button.h>
#include <soccerbot/config.h>
#include <soccerbot/differentialKinematics.h>
#include <soccerbot/lightSensor.h>
#include <soccerbot/motor.h>
#include <soccerbot/net.h>
#include <soccerbot/desc.h>
#include <soccerbot/log.h>

namespace soccerbot {

// Check if the latest OTA update has been verified
bool isOtaUpdateVerified();

// Put the robot in a known state
void setGpioOff();

// Emergency shutdown handler
void safetyShutdown();

void robotLogSink(std::string_view str);

// The state of a soccer bot
class Robot {
public:
    // Onboard button marked 0 on the S2 Mini
    Button button0;

    MOTOR_TYPE left;
    MOTOR_TYPE right;

    DifferentialKinematics kinematics;

    LightSensor frontLeft, frontRight, backLeft, backRight;

    // Outbound client to centralized server, if applicable
    TcpClient* client;

    // Task handling robot net sockets and commands
    TaskHandle_t task;

    Robot()
        : button0(GPIO_NUM_0)
        , left(PINCONFIG(MOTOR_A_PIN1), PINCONFIG(MOTOR_A_PIN2), PINCONFIG(ENCODER_A_PIN1), PINCONFIG(ENCODER_A_PIN2), FCONFIG(MOTOR_A_DRIVE_MULTIPLIER))
        , right(PINCONFIG(MOTOR_B_PIN1), PINCONFIG(MOTOR_B_PIN2), PINCONFIG(ENCODER_B_PIN1), PINCONFIG(ENCODER_B_PIN2), FCONFIG(MOTOR_B_DRIVE_MULTIPLIER))
        , kinematics(left, right)
        , frontLeft(PINCONFIG(PHOTODIODE_FRONT_LEFT))
        , frontRight(PINCONFIG(PHOTODIODE_FRONT_RIGHT))
        , backLeft(PINCONFIG(PHOTODIODE_BACK_LEFT))
        , backRight(PINCONFIG(PHOTODIODE_BACK_RIGHT))
    {
        // Shut down robot hardware when esp_restart() is called
        esp_register_shutdown_handler(safetyShutdown);

        //overrideLog();
        //addLogSink(robotLogSink);

        // Start task that polls robot net sockets and acts on their commands
        runThread();

        connectToServer();
    }

    void connectToServer() {
        auto ip = getServerIp();

        if (ip) {
            uint16_t port = 3001;

            client = addTcpClient(ip->u_addr.ip4.addr, port);
            client->waitToConnect();
            client->sendHello();

            ESP_LOGI("", "Sent HELLO to server");
        }
        else {
            ESP_LOGE("", "Choosing not to connect to server.");
        }
    }

    std::optional<ip_addr_t> getServerIp()
    {
#ifdef FIXED_SERVER_IP
        ip_addr_t addr = {};
        ip4addr_aton(FIXED_SERVER_IP, &addr.u_addr.ip4);
        addr.type = IPADDR_TYPE_V4;
        return addr;
#else
        auto domain = "soccer-server.internal";

        addrinfo* serverAddr;

        addrinfo hints = {};
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags |= AI_CANONNAME;

        auto res = getaddrinfo(domain, NULL, &hints, &serverAddr);
        if (res != 0) {
            ESP_LOGE("", "Failed to resolve server IP!");
            return {};
        }

        freeaddrinfo(serverAddr);

        in_addr ip = ((sockaddr_in*)serverAddr->ai_addr)->sin_addr;
        ip_addr_t addr = {};
        addr.u_addr.ip4.addr = ip.s_addr;
        addr.type = IPADDR_TYPE_V4;
        return addr;
#endif
    }

    void tick(uint64_t us)
    {
        left.tick(us);
        right.tick(us);
    }

    void stop()
    {
        left.set(0);
        right.set(0);
    }

    IVec2 displacements()
    {
        return { left.pos(), right.pos() };
    }

    void estop()
    {
        stop();

        // Deep sleep for a second, resetting all registers
        // This resets much more of the chip than an ordinary software reset
        // todo: set wakup reason
        esp_sleep_enable_timer_wakeup(1000000);
        esp_deep_sleep_start();
    }

    std::array<int, 4> lightLevels()
    {
        return { frontLeft.read(), frontRight.read(), backLeft.read(), backRight.read() };
    }

    void runThread();
};

extern std::optional<Robot> robot;

}; // namespace soccerbot

#endif // ifndef SOCCERBOT_ROBOT_H