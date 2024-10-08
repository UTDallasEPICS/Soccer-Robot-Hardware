#ifndef SOCCERBOT_UTIL_H
#define SOCCERBOT_UTIL_H

#include <freertos/FreeRTOS.h> // Mandatory first include
#include <freertos/task.h>

#include <esp_random.h>

#include <cstring>
#include <cmath>

#if __has_include("../../env.h")
#include "../../env.h"
#else

#ifndef WIFI_NOT_NEEDED
#error "env.h not populated"
#endif

#endif

#include <soccerbot/unit.h>
#include <soccerbot/allocation.h>

#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

namespace soccerbot {
template <typename OutT, typename InT>
OutT bitcast(InT in) noexcept
{
    static_assert(sizeof(InT) == sizeof(OutT));

    OutT out;
    std::memcpy(&out, &in, sizeof(OutT));
    return out;
}

inline bool within(float val, float range, float target)
{
    return std::abs(target - val) <= range;
}

inline bool within(int32_t val, int32_t range, int32_t target)
{
    return std::abs(target - val) <= range;
}

inline void waitForever()
{
    while (true) {
        vTaskDelay(1_s);
    }
}

// Get a number x in [low, high]
inline int randIn(int low, int high)
{
    return esp_random() % (high + 1 - low) + low;
}

inline float frand()
{
    return ((double)esp_random() / (RAND_MAX)) * 2 - 1;
}

inline int hexCharToInt(char input)
{
    if (input >= '0' && input <= '9')
        return input - '0';
    if (input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if (input >= 'a' && input <= 'f')
        return input - 'a' + 10;

    return 0;
}

// Map number in [inMin, inMax] to [outMin, outMax]
template <typename T>
inline T map(T val, T inMin, T inMax, T outMin, T outMax) {
    T slope = (outMax - outMin) / (inMax - inMin);
    return outMin + slope * (val - inMin);
}

inline float fmap(float val, float inMin, float inMax, float outMin, float outMax) {
    return map<float>(val, inMin, inMax, outMin, outMax);
}

//#define OTA_ENABLED

}; // namespace soccerbot

#endif // ifndef SOCCERBOT_UTIL_H