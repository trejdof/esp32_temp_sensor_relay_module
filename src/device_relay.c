#include "device_relay.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "RELAY";
static bool relay_state = false;

esp_err_t relay_init(void) {
    ESP_LOGI(TAG, "Initializing relay on GPIO %d", RELAY_GPIO_PIN);

    // Configure GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO %d: %s", RELAY_GPIO_PIN, esp_err_to_name(ret));
        return ret;
    }

    // Set initial state to OFF
    // Active-LOW relay: OFF = HIGH (1)
    ret = gpio_set_level(RELAY_GPIO_PIN, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set initial GPIO level: %s", esp_err_to_name(ret));
        return ret;
    }

    relay_state = false;
    ESP_LOGI(TAG, "Relay initialized successfully, state: OFF (active-LOW)");

    return ESP_OK;
}

esp_err_t relay_set_state(bool state) {
    // Active-LOW relay: invert the logic
    // state = true (ON) -> GPIO LOW (0)
    // state = false (OFF) -> GPIO HIGH (1)
    uint32_t level = state ? 0 : 1;

    esp_err_t ret = gpio_set_level(RELAY_GPIO_PIN, level);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set relay state: %s", esp_err_to_name(ret));
        return ret;
    }

    relay_state = state;
    ESP_LOGI(TAG, "Relay state changed to: %s", state ? "ON" : "OFF");

    return ESP_OK;
}

bool relay_get_state(void) {
    return relay_state;
}

esp_err_t relay_toggle(void) {
    bool new_state = !relay_state;

    ESP_LOGI(TAG, "Toggling relay from %s to %s",
             relay_state ? "ON" : "OFF",
             new_state ? "ON" : "OFF");

    return relay_set_state(new_state);
}
