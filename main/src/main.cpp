#include <freertos/FreeRTOS.h> // Mandatory first include

extern "C" void app_main()
{
    // Wait for USB serial to connect
    for (int i = 0; i < 3; i++) {
        printf("Starting...\n");
        vTaskDelay(1000);
    }

    while (true) {
        printf("Hello World!\n");
    }
}