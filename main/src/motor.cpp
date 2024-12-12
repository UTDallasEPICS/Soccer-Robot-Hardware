#include <freertos/FreeRTOS.h> // Mandatory first include

#include <soccerbot/log.h>
#include <soccerbot/motor.h>

namespace soccerbot {

Encoder::Encoder(gpio_num_t channelA_, gpio_num_t channelB_)
    : channelA(channelA_)
    , channelB(channelB_)
{
    //CHECK(rotary_encoder_init(&info, channelA, channelB));
    //CHECK(rotary_encoder_enable_half_steps(&info, true));
}
Encoder::Encoder()
    : channelA(GPIO_NUM_NC)
    , channelB(GPIO_NUM_NC)
{
    CHECK(rotary_encoder_init(&info, channelA, channelB));
    CHECK(rotary_encoder_enable_half_steps(&info, true));
}
Encoder::~Encoder()
{
    CHECK(rotary_encoder_uninit(&info));
}

// Get how far the encoder has moved since this function was last called
int32_t Encoder::getDelta()
{
    int64_t currentPos = read();

    if (abs(currentPos) > ENCODER_RESET_THRESHOLD) {
        // encoder.write(0);
    }

    int64_t dif = currentPos - lastPos;

    lastPos = currentPos;

    return dif;
}

int32_t Encoder::read()
{
    if (isValid()) { 
        rotary_encoder_state_t state = { 0 };
        CHECK(rotary_encoder_get_state(&info, &state));

        return state.position;
    }
    else {
        return 0;
    }
}

void Encoder::reset()
{
    rotary_encoder_reset(&info);
}

void Encoder::tick(uint64_t us) { }

bool Encoder::isValid() {
    return channelA != GPIO_NUM_NC && channelB != GPIO_NUM_NC;
}

MotorPwm::MotorPwm(gpio_num_t motorChannelA_,
    gpio_num_t motorChannelB_,
    gpio_num_t encoderChannelA_,
    gpio_num_t encoderChannelB_,
    float& driveMultiplier_)
    : encoder(encoderChannelA_, encoderChannelB_)
    , channelA(motorChannelA_)
    , channelB(motorChannelB_)
    , driveMultiplier(driveMultiplier_) {}

int32_t MotorPwm::pos() { 
    return encoder.read();
}

void MotorPwm::reset()
{
    set(0);
    encoder.reset();
}

void MotorPwm::tick(uint64_t us) { encoder.tick(us); }

// [-1.0, 1.0]
void MotorPwm::set(float power)
{

    ESP_LOGI("", "Setting power=%f and multiplying with driveMultiplier=%f", power, driveMultiplier);

    power *= driveMultiplier;

    if (power > 0.0) {
        // power = 1 - power;
        channelA.set(power);
        channelB.set(0.0);
        ESP_LOGI("", "Motor %f %f", power, 0.0);
    } else {
        channelA.set(0.0);
        channelB.set(-power);
        ESP_LOGI("", "Motor %f %f", 0.0, power);
    }

    // gpio_set_level(channelB, reverse);
    return;
}
}; // namespace soccerbot