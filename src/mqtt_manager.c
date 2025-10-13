#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "mqtt_client.h"  // ESP-IDF MQTT library
#include "mqtt_manager.h"  // Our header

#ifdef DEVICE_TYPE_RELAY
#include "device_relay.h"
#endif

static const char *TAG = "MQTT_CLIENT";
static esp_mqtt_client_handle_t mqtt_client = NULL;

/**
 * @brief Get the local IP address as a string
 */
static esp_err_t get_ip_address(char *ip_str, size_t max_len)
{
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif == NULL) {
        ESP_LOGE(TAG, "Failed to get netif handle");
        return ESP_FAIL;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get IP info");
        return ret;
    }

    snprintf(ip_str, max_len, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

/**
 * @brief MQTT event handler
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");

            // Publish online status with IP address
            mqtt_publish_connection_status();

            // Subscribe to topics for relay devices
            #ifdef DEVICE_TYPE_RELAY
            int msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_COMMAND, 1);
            ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", MQTT_TOPIC_COMMAND, msg_id);

            msg_id = esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_STATE_RESPONSE, 1);
            ESP_LOGI(TAG, "Subscribed to %s, msg_id=%d", MQTT_TOPIC_STATE_RESPONSE, msg_id);

            // Request current state from webapp
            ESP_LOGI(TAG, "Requesting state sync from webapp...");
            msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATE_REQUEST, "REQUEST", 7, 1, 0);
            ESP_LOGI(TAG, "State sync request sent, msg_id=%d", msg_id);
            #endif

            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

            // Handle incoming messages here
            #ifdef DEVICE_TYPE_RELAY
            // Handle state sync response
            if (strncmp(event->topic, MQTT_TOPIC_STATE_RESPONSE, event->topic_len) == 0) {
                ESP_LOGI(TAG, "Received state sync response: %.*s", event->data_len, event->data);

                // Set relay state based on response
                if (strncmp(event->data, "ON", event->data_len) == 0) {
                    relay_set_state(true);
                    ESP_LOGI(TAG, "State synced: Relay set to ON");
                } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
                    relay_set_state(false);
                    ESP_LOGI(TAG, "State synced: Relay set to OFF");
                } else {
                    ESP_LOGW(TAG, "Unknown state response: %.*s", event->data_len, event->data);
                }

                // Send sync ACK
                int ack_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATE_SYNC_ACK, "ACK", 3, 1, 0);
                ESP_LOGI(TAG, "State sync ACK sent, msg_id=%d", ack_msg_id);
            }
            // Handle normal control commands
            else if (strncmp(event->topic, MQTT_TOPIC_COMMAND, event->topic_len) == 0) {
                // Handle ON/OFF commands
                if (strncmp(event->data, "ON", event->data_len) == 0) {
                    ESP_LOGI(TAG, "Received ON command for relay");

                    // Send ACK first
                    int ack_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_ACK, "ACK", 3, 1, 0);
                    ESP_LOGI(TAG, "Sent ACK, msg_id=%d", ack_msg_id);

                    // Set relay to ON
                    relay_set_state(true);
                } else if (strncmp(event->data, "OFF", event->data_len) == 0) {
                    ESP_LOGI(TAG, "Received OFF command for relay");

                    // Send ACK first
                    int ack_msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_ACK, "ACK", 3, 1, 0);
                    ESP_LOGI(TAG, "Sent ACK, msg_id=%d", ack_msg_id);

                    // Set relay to OFF
                    relay_set_state(false);
                } else {
                    ESP_LOGW(TAG, "Unknown command: %.*s (expected ON or OFF)", event->data_len, event->data);
                }
            }
            #endif
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
            break;

        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

esp_err_t mqtt_client_init(void)
{
    // Create LWT (Last Will and Testament) message - sent when device disconnects unexpectedly
    char lwt_payload[128];
    snprintf(lwt_payload, sizeof(lwt_payload), "{\"status\":\"offline\"}");

    ESP_LOGI(TAG, "Initializing MQTT client");
    ESP_LOGI(TAG, "Broker URI: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "Device: %s (%s)", DEVICE_NAME, DEVICE_TYPE_STR);
    ESP_LOGI(TAG, "LWT Topic: %s", MQTT_TOPIC_STATUS);
    ESP_LOGI(TAG, "LWT Payload: %s", lwt_payload);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials.username = MQTT_USERNAME,
        .credentials.authentication.password = MQTT_PASSWORD,
        .network.timeout_ms = 5000,
        .session.keepalive = 20,  // 20 seconds keepalive (faster disconnect detection for testing)
        .session.last_will.topic = MQTT_TOPIC_STATUS,
        .session.last_will.msg = lwt_payload,
        .session.last_will.msg_len = strlen(lwt_payload),
        .session.last_will.qos = 1,
        .session.last_will.retain = 1,  // Retain the offline message
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t ret = esp_mqtt_client_start(mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client");
        return ret;
    }

    ESP_LOGI(TAG, "MQTT client started successfully");
    return ESP_OK;
}

esp_err_t mqtt_publish_connection_status(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_FAIL;
    }

    // Get IP address
    char ip_str[16];
    esp_err_t ret = get_ip_address(ip_str, sizeof(ip_str));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get IP address");
        strcpy(ip_str, "unknown");
    }

    // Create JSON payload
    char payload[256];
    snprintf(payload, sizeof(payload),
             "{\"status\":\"online\",\"device_type\":\"%s\",\"ip_address\":\"%s\"}",
             DEVICE_TYPE_STR, ip_str);

    ESP_LOGI(TAG, "Publishing connection status to %s", MQTT_TOPIC_STATUS);
    ESP_LOGI(TAG, "Payload: %s", payload);

    int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_STATUS, payload, 0, 1, 1);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish connection status");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Connection status published, msg_id=%d", msg_id);
    return ESP_OK;
}

esp_mqtt_client_handle_t mqtt_get_client(void)
{
    return mqtt_client;
}

void mqtt_client_stop(void)
{
    if (mqtt_client != NULL) {
        ESP_LOGI(TAG, "Stopping MQTT client");
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }
}
