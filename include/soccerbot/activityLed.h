#ifndef SOCCERBOT_ACTIVITY_LED_H
#define SOCCERBOT_ACTIVITY_LED_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <soccerbot/unit.h>

namespace soccerbot {
// Activity LED meanings by priority:
// 50ms:    OTA Update
// 5000ms:  Connected
// 1000ms:  Disconnected
constexpr TickType_t ACTIVITY_LED_DELAY_OTA = 50_ms;
constexpr TickType_t ACTIVITY_LED_DELAY_CONNECTED = 5000_ms;
constexpr TickType_t ACTIVITY_LED_DELAY_DISCONNECTED = 1000_ms;

extern bool activityLedIsOta;
extern bool activityLedIsConnected;

void startActivityLed();

void stopActivityLed();
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_ACTIVITY_LED_H