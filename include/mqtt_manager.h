#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "esp_err.h"
#include "mqtt_client.h"

/**
 * @brief Initialize and connect to MQTT broker with LWT
 *
 * This function initializes the MQTT client, sets up Last Will and Testament (LWT),
 * and connects to the broker. After successful connection, it publishes the device
 * status with IP address.
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_client_init(void);

/**
 * @brief Get the MQTT client handle
 *
 * @return esp_mqtt_client_handle_t MQTT client handle, or NULL if not initialized
 */
esp_mqtt_client_handle_t mqtt_get_client(void);

/**
 * @brief Publish device connection status (online/offline with IP)
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_publish_connection_status(void);

/**
 * @brief Disconnect from MQTT broker and cleanup
 */
void mqtt_client_stop(void);

#endif // MQTT_MANAGER_H
