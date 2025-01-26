#pragma once

// WiFi configuration - these will be provided by build flags
#ifndef WIFI_SSID
#define WIFI_SSID "default_ssid" // Fallback value
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "default_password" // Fallback value
#endif

#define WIFI_TIMEOUT 30000 // 30 seconds timeout

// Pin Definitions for ESP8266
#define LEFT_SWITCH_PIN 14  // D5
#define RIGHT_SWITCH_PIN 12 // D6
#define LED_PIN 13          // D7
#define LIVE_GIG_PIN 2      // D4 GPIO2 - Add new pin for live gig switch

// Timing Constants
#define HOLD_THRESHOLD 1000      // Long press threshold in ms
#define DEBOUNCE_TIME 50         // Debounce time in ms
#define TAP_TIMEOUT 2000         // Tap tempo timeout
#define DISPLAY_TOGGLE_TIME 3000 // Display toggle time in ms
#define LIVE_GIG_TIMEOUT 20000   // 20 seconds timeout

// Storage Constants
#define SETTINGS_ADDR 0
#define PATCHES_ADDR sizeof(Settings)
#define MAX_PATCHES 10
#define SETTINGS_CHECKSUM 0xABCD

// I2C Display Address
#define DISPLAY_ADDR 0x70