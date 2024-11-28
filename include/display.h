#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "types.h"

class Display
{
public:
    Display();
    void begin();
    void setBrightness(uint8_t brightness);
    void update(Mode currentMode, int currentPatch, const Patch *patches,
                int currentTempo, bool showingPatchName, bool wifiConnected,
                bool liveGigMode);

private:
    Adafruit_AlphaNum4 alphaDisplay;
    void writeDigitWithFlags(int position, char character, bool showDecimal);
};