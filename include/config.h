#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// Include secrets (WiFi, MQTT credentials)
// ============================================
#include "config_secrets.h"

// ============================================
// DEVICE TYPE SELECTION (uncomment ONE)
// ============================================
#define DEVICE_TYPE_RELAY
//#define DEVICE_TYPE_TEMP_SENSOR

// ============================================
// WiFi Configuration
// ============================================
#define WIFI_MAX_RETRY 5

// ============================================
// MQTT Configuration
// ============================================
#define MQTT_PORT 1883

// Device-specific MQTT topics and settings
#ifdef DEVICE_TYPE_RELAY
    #define DEVICE_NAME "relay"
    #define DEVICE_TYPE_STR "relay"
    #define MQTT_TOPIC_COMMAND "branko/boiler/control"                  // Subscribe: receives ON/OFF commands
    #define MQTT_TOPIC_ACK "branko/boiler/ack"                          // Publish: sends ACK after receiving command
    #define MQTT_TOPIC_STATUS "branko/devices/relay/status"             // Publish: device connection status
    #define MQTT_TOPIC_STATE_REQUEST "branko/boiler/state/request"      // Publish: request current state on boot
    #define MQTT_TOPIC_STATE_RESPONSE "branko/boiler/state/response"    // Subscribe: receive current state from webapp
    #define MQTT_TOPIC_STATE_SYNC_ACK "branko/boiler/state/sync_ack"    // Publish: sends ACK after state sync complete
#endif

#ifdef DEVICE_TYPE_TEMP_SENSOR
    #define DEVICE_NAME "temp_sensor"
    #define DEVICE_TYPE_STR "sensor"
    #define MQTT_TOPIC_TEMP "branko/sensor/temperature"           // Publish: temperature readings
    #define MQTT_TOPIC_STATUS "branko/devices/temp_sensor/status" // Publish: device connection status

    // I2C Configuration for AHT20 + BMP280
    #define I2C_SDA_PIN 32
    #define I2C_SCL_PIN 33
    #define I2C_FREQ_HZ 100000  // 100kHz I2C frequency

    #define TEMP_PUBLISH_INTERVAL_MS 10000  // Publish every 10 seconds
#endif

// ============================================
// General Settings
// ============================================
#define STATUS_LED_GPIO 2
#define HEARTBEAT_INTERVAL_MS 30000  // Send heartbeat every 30 seconds

#endif // CONFIG_H