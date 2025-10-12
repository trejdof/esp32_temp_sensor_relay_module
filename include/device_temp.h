#ifndef DEVICE_TEMP_H
#define DEVICE_TEMP_H

#include "esp_err.h"
#include "mqtt_client.h"

/**
 * @brief Initialize temperature sensor (mock for now)
 *
 * @return ESP_OK on success
 */
esp_err_t temp_sensor_init(void);

/**
 * @brief Read temperature from sensor (mock values for testing)
 *
 * @return float Temperature in Celsius
 */
float temp_sensor_read(void);

/**
 * @brief Start periodic temperature publishing task
 *
 * @param client MQTT client handle to use for publishing
 * @return ESP_OK on success
 */
esp_err_t temp_sensor_start_publishing(esp_mqtt_client_handle_t client);

#endif // DEVICE_TEMP_H
