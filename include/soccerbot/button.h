#ifndef SOCCERBOT_BUTTON_H
#define SOCCERBOT_BUTTON_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <stdint.h>

#include <driver/gpio.h>

namespace soccerbot {
class Button {
    gpio_num_t port_;
    uint8_t history_ = 0;
    bool justPressed_ = false;

public:
    Button(gpio_num_t port);

    void update();

    bool get();

    bool justPressed();
};
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_BUTTON_H