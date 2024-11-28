#pragma once
#include <Arduino.h>

// Operating mode enumeration
enum Mode
{
    PATCH_MODE,
    FREE_MODE
};

// Settings structure
struct Settings
{
    bool liveGigMode;
    uint8_t brightness;
    uint32_t checksum;
};

// Patch structure
struct Patch
{
    char name[5]; // 4 chars + null terminator
    int tempo;
};

// Button state structure
struct Button
{
    int pin;
    bool lastState;
    bool currentState;
    unsigned long lastDebounceTime;
    unsigned long pressStartTime;
    bool isLongPress;

    Button(int _pin) : pin(_pin),
                       lastState(HIGH),
                       currentState(HIGH),
                       lastDebounceTime(0),
                       pressStartTime(0),
                       isLongPress(false) {}
};