#include "display.h"
#include "config.h"

Display::Display() : alphaDisplay()
{
}

void Display::begin()
{
    alphaDisplay.begin(DISPLAY_ADDR);
}

void Display::setBrightness(uint8_t brightness)
{
    alphaDisplay.setBrightness(brightness);
}

void Display::writeDigitWithFlags(int position, char character, bool showDecimal)
{
    alphaDisplay.writeDigitAscii(position, character, showDecimal);
}

void Display::update(Mode currentMode, int currentPatch, const Patch *patches,
                     int currentTempo, bool showingPatchName, bool wifiConnected,
                     bool liveGigMode)
{
    // Clear all decimal points first
    for (int i = 0; i < 4; i++)
    {
        alphaDisplay.writeDigitRaw(i, 0);
    }

    if (currentMode == PATCH_MODE)
    {
        if (showingPatchName)
        {
            for (int i = 0; i < 4; i++)
            {
                // WiFi status on last decimal, Live mode status on first decimal
                bool showDecimal = (wifiConnected && i == 3) || (liveGigMode && i == 0);
                writeDigitWithFlags(i, patches[currentPatch].name[i], showDecimal);
            }
        }
        else
        {
            String tempoStr = String(patches[currentPatch].tempo);
            while (tempoStr.length() < 4)
                tempoStr = " " + tempoStr;
            for (int i = 0; i < 4; i++)
            {
                bool showDecimal = (wifiConnected && i == 3) || (liveGigMode && i == 0);
                writeDigitWithFlags(i, tempoStr[i], showDecimal);
            }
        }
    }
    else
    {
        // In FREE_MODE
        writeDigitWithFlags(0, 'F', liveGigMode); // Show decimal on F if in live mode
        String tempoStr = String(currentTempo);
        while (tempoStr.length() < 3)
            tempoStr = " " + tempoStr;
        for (int i = 0; i < 3; i++)
        {
            writeDigitWithFlags(i + 1, tempoStr[i], wifiConnected && (i == 2));
        }
    }

    alphaDisplay.writeDisplay();
}