#include <freertos/FreeRTOS.h> // Mandatory first include

#include <esp_log.h>

#include <charconv>
#include <ArduinoJson.h>

#include <soccerbot/robot.h>
#include <soccerbot/update.h>

#include <system_error>

namespace soccerbot {

// On its thread, a robot parses commands and acts on them
void robotThread(void* robotPtr)
{
    Robot& bot = *(Robot*)(robotPtr);
    JsonDocument json;

    ESP_LOGI("", "j1");

    while (true) {
        runSockets();

        for (int i = 0; i < clientsCount; i++) {
            TcpClient* client = clients[i];

            if (client == nullptr) {
                ESP_LOGI("", "TcpClient is null");
                continue;
            }

            while (client->hasPacket()) {
                std::string_view str = client->readPacket();

                // Read JSON packet
                DeserializationError error = deserializeJson(json, str);

                if (error) {
                    ESP_LOGE("", "deserializeJson() failed: %s", error.c_str());
                    client->clearPacket();
                    continue;
                }

                auto type = json["type"];

                #if MOTOR_TYPE == MotorDesc 
                if (type == "OVERRIDE") {
                    int left = json["left"].as<int>();
                    int right = json["right"].as<int>();
                    ESP_LOGI("", "Driving that pwm! %d %d", left, right);
                    bot.left.pin.setDuty(left);
                    bot.right.pin.setDuty(right);
                }
                #endif
                
                if (type == "DRIVE_TANK") {
                    float left = json["left"].as<float>();
                    float right = json["right"].as<float>();
                    ESP_LOGI("", "Driving that tank! %f %f", left, right);
                    bot.left.set(left);
                    bot.right.set(right);
                } else if (type == "PING_SEND") {
                    char pingResponse[] = R"({"type":"PING_RESPONSE"};)";
                    client->send(pingResponse);
                } else if (type == "SET_VAR") {
                    ConfigKey key = (ConfigKey)(json["key"].as<int>());
                    auto varType = json["var_type"];

                    if (varType == "int32") {
                        setConfig<int32_t>(key, json["val"].as<int32_t>());
                    } else if (varType == "uint32") {
                        setConfig<uint32_t>(key, json["val"].as<uint32_t>());
                    } else if (varType == "float") {
                        setConfig<float>(key, json["val"].as<float>());
                    }
                } else if (type == "QUERY_VAR") {
                    ConfigKey key = (ConfigKey)json["var_id"].as<int>();
                    auto varType = json["var_type"];

                    char varBuf[100];
                    int sz = 0;

                    if (varType == "int32") {
                        static_assert(sizeof(int) == sizeof(int32_t)); // No Arduino here
                        int varVal = getConfig<int32_t>(key);
                        sz = snprintf(varBuf, sizeof(varBuf), R"({"type":"QUERY_RESPONSE","var_val":%d};)", varVal);
                    } else if (varType == "uint32") {
                        static_assert(sizeof(int) == sizeof(uint32_t));
                        uint32_t varVal = getConfig<uint32_t>(key);
                        sz = snprintf(varBuf, sizeof(varBuf), R"({"type":"QUERY_RESPONSE","var_val":%u};)", (unsigned int)varVal);
                    } else if (varType == "float") {
                        float varVal = getConfig<float>(key);
                        sz = snprintf(varBuf, sizeof(varBuf), R"({"type":"QUERY_RESPONSE","var_val":%f};)", varVal);
                    } else {
                        ESP_LOGE("", "Bad QUERY_VAR");
                    }

                    client->send(std::string_view(varBuf, sz));
                } else if (type == "SERVER_HELLO") {
                    uint32_t version = json["protocol"];
                    (void)version;

                    JsonArray config = json["config"];

                    ConfigKey key = ConfigKey(0);
                    for (JsonVariantConst i : config) {
                        std::string_view type = i[0];
                        std::string_view value = i[1];

                        if (type == "gpio" || type == "int32") {
                            int val = -1;
                            auto [ptr, ec] = std::from_chars(value.begin(), value.end(), val);
                            
                            ESP_LOGI("", "Type: %s, Value: %s", type.data(), value.data());
                            ESP_LOGI("", "EC: %s", std::generic_category().message(static_cast<int>(ec)).c_str());
                            
                            CHECK(ec == std::errc());
                            
                            setConfig(key, val);
                        }
                        else if (type == "float") {
                            float val = -1;
                            auto [ptr, ec] = std::from_chars(value.begin(), value.end(), val);
                            
                            ESP_LOGI("", "Type: %s, Value: %s", type.data(), value.data());
                            ESP_LOGI("", "EC: %s", std::generic_category().message(static_cast<int>(ec)).c_str());
                            
                            CHECK(ec == std::errc());
                            
                            setConfig(key, val);
                        }
                        else {
                            CHECK(false);
                        }

                        key = ConfigKey(int(key) + 1);
                    }
                }

                client->clearPacket();
            }
        }

        vTaskDelay(10_ms);
    }
}

void Robot::runThread()
{
    CHECK(xTaskCreate(robotThread, "robotThread", TaskStackSize::LARGE, (void*)this, TaskPriority::ROBOT, &this->task) == pdPASS);
}

}; // namespace soccerbot