#include <mbedtls/base64.h>
#include <esp_debug_helpers.h>

#include <soccerbot/robot.h>

namespace soccerbot {

// Check if the latest OTA update has been verified
bool isOtaUpdateVerified()
{
#ifdef OTA_ENABLED
    esp_ota_img_states_t state;

    CHECK(esp_ota_get_state_partition(esp_ota_get_running_partition(), &state));

    if (state == ESP_OTA_IMG_PENDING_VERIFY) {
        return false;
    }
#endif

    return true;
}

// Put the robot in a known state
void setGpioOff()
{
    if (isOtaUpdateVerified()) {
        // Don't do potentially dangerous GPIO while testing a new update
        return;
    }

    gpio_set_level(PINCONFIG(MOTOR_A_PIN1), 0);
    gpio_set_level(PINCONFIG(MOTOR_A_PIN2), 0);
    gpio_set_level(PINCONFIG(MOTOR_B_PIN1), 0);
    gpio_set_level(PINCONFIG(MOTOR_B_PIN2), 0);

    gpio_set_level(PINCONFIG(RELAY_IR_LED), 0);
}

// Emergency shutdown handler
void safetyShutdown()
{
    setGpioOff();
}

// Builds a network-ready log, with base64 encoded raw data and a JSON wrapper
// Implicitly guarded by logMutex
static char logBuf[MAX_LOG_SIZE + 1];

void robotLogSink(std::string_view str)
{
    printf("k1\n");

    esp_backtrace_print(999);

    char* bufEnd = logBuf + sizeof(logBuf);
    char* bufItr = logBuf;

    bufItr += abs(snprintf(bufItr, bufEnd - bufItr, R"({"type":"LOG","msg":")"));
    if (bufItr >= (bufEnd - 2)) { return; }

    printf("k2\n");

    size_t encodeBytesWritten = 0;
    CHECK(mbedtls_base64_encode((unsigned char*)bufItr, 0, &encodeBytesWritten, (const unsigned char*)str.data(), str.size()) == 0);
    CHECK(encodeBytesWritten < (bufEnd - bufItr));

    CHECK(mbedtls_base64_encode((unsigned char*)bufItr, bufItr - logBuf, &encodeBytesWritten, (const unsigned char*)str.data(), str.size()) == 0);
    CHECK(encodeBytesWritten < 1000);
    bufItr += encodeBytesWritten;

    /*
    for (char c : str) {
        if (c == '\"' || c == '\\' || c == '\n') {
            // Encode non-basic characters (mostly just double quote and newline)
            bufItr += abs(snprintf(bufItr, bufEnd - bufItr, "\\x%02X", c));
        }
        else if (isalnum(c)) {
            // Strip out color codes (wish I knew how to include them)
            *bufItr++ = c;
        }

        if (bufItr >= (bufEnd - 2)) { return; }
    }
    */

    bufItr += abs(snprintf(bufItr, bufEnd - bufItr, "\"};"));
    if (bufItr >= (bufEnd - 2)) { return; }

    printf("k4\n");

    for (int i = 0; i < clientsCount; i++) {
        TcpClient* client = clients[i];

        if (client == nullptr) {
            continue;
        }
        printf("k5\n");

        client->send(std::string_view(logBuf, bufItr - logBuf));
    }
}

std::optional<Robot> robot;

};