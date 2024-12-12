#ifndef SOCCERBOT_DESC_H
#define SOCCERBOT_DESC_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <driver/ledc.h>
#include <driver/gpio.h>

#include "soccerbot/dac.h"

namespace soccerbot {

// Brushed Electronic Speed Controller
class MotorDesc {
    public:
    constexpr static uint32_t FORWARD_MIN = 1295;
    constexpr static uint32_t FORWARD_MAX = 1380;
    constexpr static uint32_t REVERSE_MIN = 1100;
    constexpr static uint32_t REVERSE_MAX = 1160;
    constexpr static uint32_t NEUTRAL = 1229;

    constexpr static uint32_t TIMER_WIDTH_TICKS = 1024;

    PwmPin pin;
    float& driveMultiplier;

    MotorDesc(gpio_num_t motorChannelA_,
        gpio_num_t motorChannelB_,
        gpio_num_t encoderChannelA_,
        gpio_num_t encoderChannelB_,
        float& driveMultiplier_);
    ~MotorDesc();

    // Set speed in [-0.0, 1.0]
    void set(float power);

    void stop();

    void start();

    int32_t pos();

    void tick(uint64_t us);
};

}; // namespace soccerbot

#endif // ifndef SOCCERBOT_DESC_H