#ifndef DEVICE_TEMP_H
#define DEVICE_TEMP_H

#include "esp_err.h"
#include "mqtt_client.h"

/**
 * @brief Sensor reading structure
 */
typedef struct {
    float aht20_temp;      // Temperature from AHT20 (°C)
    float aht20_humidity;  // Humidity from AHT20 (%)
    bool aht20_valid;      // AHT20 reading valid
} sensor_data_t;

/**
 * @brief Initialize I2C and temperature sensor (AHT20)
 *
 * @return ESP_OK on success
 */
esp_err_t temp_sensor_init(void);

/**
 * @brief Read data from AHT20 sensor
 *
 * @param data Pointer to sensor_data_t structure to store readings
 * @return ESP_OK on success
 */
esp_err_t temp_sensor_read(sensor_data_t *data);

/**
 * @brief Start periodic temperature publishing task
 *
 * @param client MQTT client handle to use for publishing
 * @return ESP_OK on success
 */
esp_err_t temp_sensor_start_publishing(esp_mqtt_client_handle_t client);

#endif // DEVICE_TEMP_H
