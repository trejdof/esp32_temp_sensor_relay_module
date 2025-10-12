#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "esp_err.h"

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
 * @brief Publish device connection status (online/offline with IP)
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t mqtt_publish_connection_status(void);

/**
 * @brief Disconnect from MQTT broker and cleanup
 */
void mqtt_client_stop(void);

#endif // MQTT_CLIENT_H
