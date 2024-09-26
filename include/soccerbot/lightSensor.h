#ifndef SOCCERBOT_LIGHT_SENSOR_H
#define SOCCERBOT_LIGHT_SENSOR_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <soccerbot/adc.h>

namespace soccerbot {
class LightSensor {
    adc_channel_t channel;

public:
    LightSensor(gpio_num_t channel_)
        : channel((adc_channel_t)channel_)
    {
        adcInitPin(channel);
    }

    int read()
    {
        return adcRead(channel);
    }
};
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_LIGHT_SENSOR_H