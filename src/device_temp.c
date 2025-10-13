#include "config.h"

#ifdef DEVICE_TYPE_TEMP_SENSOR

#include <stdio.h>
#include <stdlib.h>
#include "device_temp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"  // ESP-IDF MQTT library

static const char *TAG = "TEMP_SENSOR";
static esp_mqtt_client_handle_t mqtt_client = NULL;

// Mock temperature - cycles between 20.0 and 21.0 in 0.1 steps
static float current_temp = 20.0;
static bool temp_increasing = true;

esp_err_t temp_sensor_init(void)
{
    ESP_LOGI(TAG, "Initializing mock temperature sensor");
    ESP_LOGI(TAG, "Initial temperature: %.1fC", current_temp);
    return ESP_OK;
}

float temp_sensor_read(void)
{
    // Cycle temperature between 20.0 and 21.0 in 0.1 degree steps
    if (temp_increasing) {
        current_temp += 0.1;
        if (current_temp >= 21.0) {
            current_temp = 21.0;
            temp_increasing = false;
        }
    } else {
        current_temp -= 0.1;
        if (current_temp <= 20.0) {
            current_temp = 20.0;
            temp_increasing = true;
        }
    }

    ESP_LOGI(TAG, "Mock temperature reading: %.1fC", current_temp);
    return current_temp;
}

static void publish_temperature(float temp)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not set");
        return;
    }

    char payload[16];
    snprintf(payload, sizeof(payload), "%.1f", temp);

    ESP_LOGI(TAG, "Publishing temperature: %sC to %s", payload, MQTT_TOPIC_TEMP);

    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMP, payload, 0, 0, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish temperature");
    } else {
        ESP_LOGI(TAG, "Temperature published successfully, msg_id=%d", msg_id);
    }
}

static void temperature_task(void *pvParameters)
{
    esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvParameters;
    mqtt_client = client;

    ESP_LOGI(TAG, "Temperature publishing task started");
    ESP_LOGI(TAG, "Publishing interval: %d ms", TEMP_PUBLISH_INTERVAL_MS);

    // Wait a bit for MQTT to connect
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (1) {
        float temp = temp_sensor_read();
        publish_temperature(temp);

        vTaskDelay(pdMS_TO_TICKS(TEMP_PUBLISH_INTERVAL_MS));
    }
}

esp_err_t temp_sensor_start_publishing(esp_mqtt_client_handle_t client)
{
    if (client == NULL) {
        ESP_LOGE(TAG, "Invalid MQTT client handle");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Starting temperature publishing task");

    BaseType_t ret = xTaskCreate(
        temperature_task,
        "temp_task",
        4096,
        (void *)client,
        5,
        NULL
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create temperature task");
        return ESP_FAIL;
    }

    return ESP_OK;
}

#endif // DEVICE_TYPE_TEMP_SENSOR
