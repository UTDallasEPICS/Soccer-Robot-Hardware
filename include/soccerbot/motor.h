#ifndef SOCCERBOT_MOTOR_H
#define SOCCERBOT_MOTOR_H

#include <driver/gpio.h>

#include <rotary_encoder.h>

#include <soccerbot/dac.h>

namespace soccerbot {
// Wrapper for an encoder of any type
class Encoder {
public:
    // Threshold for resetting the encoder
    constexpr static int32_t ENCODER_RESET_THRESHOLD = 2147483647 / 2;

    gpio_num_t channelA, channelB;
    //::Encoder encoder;

    int64_t lastPos = 0;

    rotary_encoder_info_t info;

    Encoder(gpio_num_t channelA_, gpio_num_t channelB_);
    ~Encoder();

    // Get how far the encoder has moved since this function was last called
    int32_t getDelta();

    int32_t read();

    void reset();

    void tick(uint64_t us);
};

class Motor {
public:
    Encoder* encoder;
    // PwmPin powerPin;

    PwmPin channelA;
    PwmPin channelB;

    float driveMultiplier;

    Motor(gpio_num_t motorChannelA_,
        gpio_num_t motorChannelB_,
        gpio_num_t encoderChannelA_,
        gpio_num_t encoderChannelB_,
        float driveMultiplier_ = 1.0);

    Motor(gpio_num_t motorChannelA_, gpio_num_t motorChannelB_, float driveMultiplier_ = 1.0);

    int32_t pos();

    void reset();

    void tick(uint64_t us);

    // [-1.0, 1.0]
    void set(float power);
};
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_MOTOR_H