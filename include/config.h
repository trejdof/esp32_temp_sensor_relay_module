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
    #define DEVICE_NAME "relay_1"
    #define MQTT_TOPIC_COMMAND "home/relay_1/command"    // Subscribe: receives ON/OFF
    #define MQTT_TOPIC_STATE "home/relay_1/state"        // Publish: sends current state
    #define RELAY_GPIO 2
#endif

#ifdef DEVICE_TYPE_TEMP_SENSOR
    #define DEVICE_NAME "temp_sensor_1"
    #define MQTT_TOPIC_TEMP "home/temp_sensor_1/temperature"
    #define MQTT_TOPIC_STATUS "home/temp_sensor_1/status"
    #define TEMP_SENSOR_GPIO 4
    #define TEMP_PUBLISH_INTERVAL_MS 10000  // Publish every 10 seconds
#endif

// ============================================
// General Settings
// ============================================
#define STATUS_LED_GPIO 2
#define HEARTBEAT_INTERVAL_MS 30000  // Send heartbeat every 30 seconds

#endif // CONFIG_H