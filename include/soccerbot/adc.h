#ifndef SOCCERBOT_ADC_H
#define SOCCERBOT_ADC_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <esp_adc/adc_oneshot.h>

namespace soccerbot {
esp_err_t initAdc();

void adcInitPin(adc_channel_t channel, adc_atten_t atten = ADC_ATTEN_DB_12);

// 0-2,200mV
int adcRead(adc_channel_t channel);
}; // namespace soccerbot

#endif // ifndef SOCCERBOT_ADC_H