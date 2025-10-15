#include "config.h"

#ifdef DEVICE_TYPE_TEMP_SENSOR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "device_temp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"
#include "driver/i2c.h"

static const char *TAG = "TEMP_SENSOR";
static esp_mqtt_client_handle_t mqtt_client = NULL;

// I2C addresses
#define AHT20_I2C_ADDR      0x38

// AHT20 commands
#define AHT20_CMD_INIT      0xBE
#define AHT20_CMD_TRIGGER   0xAC
#define AHT20_CMD_SOFTRESET 0xBA

// I2C timeout
#define I2C_TIMEOUT_MS 1000

static bool aht20_initialized = false;

// I2C helper functions
static void i2c_scanner(void)
{
    ESP_LOGI(TAG, "Scanning I2C bus...");
    uint8_t devices_found = 0;

    for (uint8_t addr = 1; addr < 127; addr++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "  Found device at address 0x%02X", addr);
            devices_found++;
        }
    }

    if (devices_found == 0) {
        ESP_LOGW(TAG, "No I2C devices found! Check your wiring:");
        ESP_LOGW(TAG, "  - SDA connected to GPIO%d", I2C_SDA_PIN);
        ESP_LOGW(TAG, "  - SCL connected to GPIO%d", I2C_SCL_PIN);
        ESP_LOGW(TAG, "  - Both sensors powered (3.3V and GND)");
        ESP_LOGW(TAG, "  - Pull-up resistors on SDA/SCL (if needed)");
    } else {
        ESP_LOGI(TAG, "I2C scan complete. Found %d device(s)", devices_found);
    }
}

static esp_err_t i2c_master_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };

    esp_err_t err = i2c_param_config(I2C_NUM_0, &conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
        return err;
    }

    err = i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C initialized on SDA=%d, SCL=%d", I2C_SDA_PIN, I2C_SCL_PIN);
    return ESP_OK;
}

static esp_err_t i2c_write_cmd(uint8_t addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    if (len > 0) {
        i2c_master_write(cmd, data, len, true);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t i2c_read_reg(uint8_t addr, uint8_t reg, uint8_t *data, size_t len)
{
    esp_err_t ret;

    // Write register address
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        return ret;
    }

    // Read data
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    return ret;
}

static esp_err_t i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_write_cmd(addr, data, 2);
}

// AHT20 functions
static esp_err_t aht20_init(void)
{
    ESP_LOGI(TAG, "Initializing AHT20...");

    vTaskDelay(pdMS_TO_TICKS(40)); // Wait for sensor to be ready

    // Send soft reset
    uint8_t reset_cmd = AHT20_CMD_SOFTRESET;
    esp_err_t ret = i2c_write_cmd(AHT20_I2C_ADDR, &reset_cmd, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT20 soft reset failed: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(20));

    // Initialize sensor
    uint8_t init_cmd[3] = {AHT20_CMD_INIT, 0x08, 0x00};
    ret = i2c_write_cmd(AHT20_I2C_ADDR, init_cmd, 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT20 init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "AHT20 initialized successfully!");
    return ESP_OK;
}

static esp_err_t aht20_read(float *temperature, float *humidity)
{
    // Trigger measurement
    uint8_t trigger_cmd[3] = {AHT20_CMD_TRIGGER, 0x33, 0x00};
    esp_err_t ret = i2c_write_cmd(AHT20_I2C_ADDR, trigger_cmd, 3);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT20 trigger failed: %s", esp_err_to_name(ret));
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(80)); // Wait for measurement

    // Read data
    uint8_t data[7];
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AHT20_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 6, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &data[6], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(I2C_TIMEOUT_MS));
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "AHT20 read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Check if busy
    if (data[0] & 0x80) {
        ESP_LOGW(TAG, "AHT20 busy");
        return ESP_ERR_INVALID_STATE;
    }

    // Calculate humidity
    uint32_t raw_humidity = ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | ((data[3] >> 4) & 0x0F);
    *humidity = (raw_humidity * 100.0) / 1048576.0;

    // Calculate temperature
    uint32_t raw_temp = (((uint32_t)data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];
    *temperature = ((raw_temp * 200.0) / 1048576.0) - 50.0;

    return ESP_OK;
}

// Public API
esp_err_t temp_sensor_init(void)
{
    ESP_LOGI(TAG, "Initializing I2C and sensors...");
    ESP_LOGI(TAG, "I2C SDA: GPIO%d, SCL: GPIO%d", I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize I2C
    esp_err_t ret = i2c_master_init();
    if (ret != ESP_OK) {
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // Scan I2C bus to see what devices are connected
    i2c_scanner();

    // Initialize AHT20
    if (aht20_init() == ESP_OK) {
        aht20_initialized = true;
    } else {
        ESP_LOGW(TAG, "AHT20 sensor not initialized! Will continue without sensor data.");
        ESP_LOGW(TAG, "Check wiring:");
        ESP_LOGW(TAG, "  SDA -> GPIO%d", I2C_SDA_PIN);
        ESP_LOGW(TAG, "  SCL -> GPIO%d", I2C_SCL_PIN);
        // Return OK anyway so the program doesn't crash
        return ESP_OK;
    }

    ESP_LOGI(TAG, "AHT20 sensor initialization complete");
    return ESP_OK;
}

esp_err_t temp_sensor_read(sensor_data_t *data)
{
    if (data == NULL) {
        ESP_LOGE(TAG, "NULL data pointer");
        return ESP_FAIL;
    }

    // Initialize all fields
    memset(data, 0, sizeof(sensor_data_t));

    // Read AHT20
    if (aht20_initialized) {
        if (aht20_read(&data->aht20_temp, &data->aht20_humidity) == ESP_OK) {
            data->aht20_valid = true;
            ESP_LOGI(TAG, "AHT20 - Temperature: %.2f°C, Humidity: %.2f%%",
                     data->aht20_temp, data->aht20_humidity);
        } else {
            ESP_LOGE(TAG, "Failed to read from AHT20");
        }
    }

    return data->aht20_valid ? ESP_OK : ESP_FAIL;
}

static void publish_temperature(sensor_data_t *data)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not set");
        return;
    }

    if (!data->aht20_valid) {
        ESP_LOGE(TAG, "No valid sensor data to publish");
        return;
    }

    // Send simple float string (webapp expects: float(payload))
    char payload[16];
    snprintf(payload, sizeof(payload), "%.2f", data->aht20_temp);

    ESP_LOGI(TAG, "Publishing temperature to %s: %s°C", MQTT_TOPIC_TEMP, payload);

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

    sensor_data_t data;

    while (1) {
        ESP_LOGI(TAG, "Reading sensors...");

        if (temp_sensor_read(&data) == ESP_OK) {
            publish_temperature(&data);
        } else {
            ESP_LOGE(TAG, "Failed to read sensor data");
        }

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
