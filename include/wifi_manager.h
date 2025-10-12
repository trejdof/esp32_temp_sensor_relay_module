#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize WiFi manager and configure WiFi station mode
 *
 * This function initializes the WiFi subsystem, registers event handlers,
 * and starts the WiFi connection process. It uses credentials from config_secrets.h
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Wait for WiFi connection to complete
 *
 * This function blocks until either the WiFi connection succeeds or fails
 * after maximum retry attempts.
 *
 * @return ESP_OK if connected successfully, ESP_FAIL if connection failed
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Check if WiFi is currently connected
 *
 * @return true if WiFi is connected, false otherwise
 */
bool wifi_manager_is_connected(void);

#endif // WIFI_MANAGER_H
