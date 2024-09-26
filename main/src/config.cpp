#include <freertos/FreeRTOS.h> // Mandatory first include

#include <hal/adc_types.h>

#include <soccerbot/config.h>

namespace soccerbot {
uint32_t configStore[(size_t)ConfigKey::CONFIG_COUNT] = {};
}; // namespace soccerbot