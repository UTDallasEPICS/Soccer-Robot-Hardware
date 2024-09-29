#ifndef SOCCERBOT_MDNS_H
#define SOCCERBOT_MDNS_H

#include <freertos/FreeRTOS.h> // Mandatory first include

#include <mdns.h>

#include <soccerbot/util.h>

namespace soccerbot {

inline void startMdnsAdvertise()
{
    // Init service
    CHECK(mdns_init());

    mdns_hostname_set("my-esp32");
    mdns_instance_name_set("Esp32 Instance Name");

    //mdns_service_add(NULL, "_jsonbot", 80, NULL, 0);
}

}; // namespace soccerbot

#endif // ifndef SOCCERBOT_MDNS_H