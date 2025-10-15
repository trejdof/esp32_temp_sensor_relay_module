#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "config.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

#ifdef DEVICE_TYPE_TEMP_SENSOR
#include "device_temp.h"
#endif

#ifdef DEVICE_TYPE_RELAY
#include "device_relay.h"
#endif

static const char *TAG = "MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "\n\n========================================");
    ESP_LOGI(TAG, "ESP32 Device - %s", DEVICE_NAME);
    ESP_LOGI(TAG, "========================================\n");

    // Initialize NVS (required for WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize and connect WiFi
    ESP_ERROR_CHECK(wifi_manager_init());
    ESP_ERROR_CHECK(wifi_manager_connect());

    // Initialize MQTT client with LWT and announce connection
    ESP_LOGI(TAG, "Initializing MQTT...");
    ESP_ERROR_CHECK(mqtt_client_init());

    // Device-specific initialization based on config.h
#ifdef DEVICE_TYPE_RELAY
    ESP_LOGI(TAG, "Device Type: RELAY SWITCH");
    ESP_LOGI(TAG, "GPIO: %d", RELAY_GPIO_PIN);

    // Initialize relay
    ESP_ERROR_CHECK(relay_init());

    ESP_LOGI(TAG, "Relay initialized and ready to receive TOGGLE commands via MQTT");
#endif

#ifdef DEVICE_TYPE_TEMP_SENSOR
    ESP_LOGI(TAG, "Device Type: TEMPERATURE SENSOR");
    ESP_LOGI(TAG, "I2C SDA: GPIO%d, SCL: GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);
    ESP_LOGI(TAG, "Publish Interval: %d ms", TEMP_PUBLISH_INTERVAL_MS);

    // Initialize temperature sensor
    ESP_ERROR_CHECK(temp_sensor_init());

    // Start publishing temperature readings
    ESP_LOGI(TAG, "Starting temperature publishing task...");
    ESP_ERROR_CHECK(temp_sensor_start_publishing(mqtt_get_client()));
#endif

    // Main loop
    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}