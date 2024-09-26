#ifndef SOCCERBOT_UPDATE_H
#define SOCCERBOT_UPDATE_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <stdint.h>

namespace soccerbot {
// Create the low-priority task to occasionally check for OTA updates
void launchUpdater();

// Skip the update delay
void checkUpdateNow();
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_UPDATE_H