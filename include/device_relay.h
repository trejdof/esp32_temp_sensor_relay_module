#ifndef DEVICE_RELAY_H
#define DEVICE_RELAY_H

#include <stdbool.h>
#include "esp_err.h"

// Relay GPIO pin (IO27 / D27)
#define RELAY_GPIO_PIN 27

/**
 * @brief Initialize the relay GPIO
 *
 * Configures GPIO2 as output and sets initial state to OFF
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t relay_init(void);

/**
 * @brief Set relay state
 *
 * @param state true to turn relay ON, false to turn relay OFF
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t relay_set_state(bool state);

/**
 * @brief Get current relay state
 *
 * @return true if relay is ON, false if relay is OFF
 */
bool relay_get_state(void);

/**
 * @brief Toggle relay state
 *
 * Switches relay from ON to OFF or OFF to ON
 *
 * @return ESP_OK on success, ESP_FAIL on error
 */
esp_err_t relay_toggle(void);

#endif // DEVICE_RELAY_H
