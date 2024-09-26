#ifndef SOCCERBOT_UNIT_H
#define SOCCERBOT_UNIT_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <freertos/portmacro.h>

namespace soccerbot {
constexpr TickType_t operator""_ms(unsigned long long int time)
{
    return pdMS_TO_TICKS(time);
}

constexpr TickType_t operator""_s(unsigned long long int time)
{
    // seconds * seconds^-1
    return time * CONFIG_FREERTOS_HZ;
}
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_UNIT_H