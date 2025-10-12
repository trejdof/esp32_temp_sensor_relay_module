# ESP32 Temperature Sensor and Relay Module

ESP32-based temperature monitoring and relay control system with MQTT communication capabilities.

## Features

- Temperature sensor monitoring
- Relay control
- WiFi connectivity
- MQTT communication for remote monitoring and control
- Built with ESP-IDF framework via PlatformIO

## Prerequisites

- [PlatformIO](https://platformio.org/) (PlatformIO Core CLI or IDE extension for VS Code)
- ESP32 development board
- USB cable for programming and power

## Setup Instructions

### 1. Clone the Repository

```bash
git clone https://github.com/trejdof/esp32_temp_sensor_relay_module.git
cd esp32_temp_sensor_relay_module
```

### 2. Configure Credentials

Create your secrets configuration file:

```bash
cp include/config_secrets.h.template include/config_secrets.h
```

Edit `include/config_secrets.h` and replace the placeholder values with your actual credentials:

```c
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"
#define MQTT_BROKER_URI "mqtt://YOUR_BROKER_ADDRESS"
#define MQTT_USERNAME "YOUR_MQTT_USERNAME"  // Leave as "" if not required
#define MQTT_PASSWORD "YOUR_MQTT_PASSWORD"  // Leave as "" if not required
```

**Note:** The `config_secrets.h` file is gitignored and will never be committed to the repository.

### 3. Configure Upload Port

Edit `platformio.ini` and update the COM port to match your system:

```ini
upload_port = COM5      # Change to your port (e.g., COM3 on Windows, /dev/ttyUSB0 on Linux)
monitor_port = COM5     # Change to match upload_port
```

### 4. Build and Upload

Using PlatformIO CLI:

```bash
# Build the project
pio run

# Upload to ESP32
pio run -t upload

# Monitor serial output
pio run -t monitor
```

Or use the PlatformIO IDE buttons in VS Code.

## Project Structure

```
esp32_temp_sensor_relay_module/
├── include/              # Header files
│   ├── config.h
│   ├── config_secrets.h.template
│   ├── device_relay.h
│   ├── device_temp.h
│   ├── mqtt_client.h
│   └── wifi_manager.h
├── src/                  # Source files
│   ├── main.c
│   ├── device_relay.c
│   ├── device_temp.c
│   ├── mqtt_client.c
│   └── wifi_manager.c
├── platformio.ini        # PlatformIO configuration
└── README.md
```

## MQTT Integration

This module is designed to work with the [mqtt-webapp](https://github.com/trejdof/mqtt-webapp) for remote monitoring and control via a web interface.
