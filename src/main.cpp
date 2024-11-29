#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "config.h"
#include "types.h"
#include "display.h"
#include "buttons.h"
#include "storage.h"
#include "metronome.h"
#include "wifi_manager.h"

Display display;
Buttons buttons;
Metronome metronome;

// Global state
Mode currentMode = PATCH_MODE;
int currentPatch = 0;
bool showingPatchName = true;
Settings settings;
Patch patches[MAX_PATCHES];
WiFiManager wifiManager(patches, settings, display); // Pass references to constructor
unsigned long lastActivityTime = 0;
bool displayActive = true;
unsigned long lastDisplayToggle = 0;

void updateActivity()
{
  lastActivityTime = millis();
  displayActive = true;
}

void checkDisplayTimeout()
{
  if (settings.liveGigMode && (millis() - lastActivityTime > LIVE_GIG_TIMEOUT))
  {
    displayActive = false;
  }
}

void handleDisplayToggle()
{
  if (currentMode == PATCH_MODE)
  {
    unsigned long currentTime = millis();
    if (currentTime - lastDisplayToggle >= DISPLAY_TOGGLE_TIME)
    {
      showingPatchName = !showingPatchName;
      lastDisplayToggle = currentTime;
      display.update(currentMode, currentPatch, patches,
                     metronome.getTempo(),
                     showingPatchName,
                     wifiManager.isConnected(),
                     settings.liveGigMode);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("\nStarting Metronome...");

  Wire.begin();

  display.begin();
  buttons.begin();
  storage.begin();
  metronome.begin();
  wifiManager.begin();

  settings = storage.loadSettings();
  storage.loadPatches(patches, MAX_PATCHES);

  display.setBrightness(settings.brightness);
  metronome.setLiveGigMode(settings.liveGigMode);
  metronome.setTempo(patches[currentPatch].tempo);

  updateActivity();
  lastDisplayToggle = millis();

  display.update(currentMode, currentPatch, patches,
                 metronome.getTempo(),
                 showingPatchName,
                 wifiManager.isConnected(),
                 settings.liveGigMode);
}

void loop()
{
  wifiManager.update();

  if (buttons.update())
  {
    updateActivity();

    if (buttons.isLeftLongPress())
    {
      currentMode = (currentMode == PATCH_MODE) ? FREE_MODE : PATCH_MODE;
      showingPatchName = true;
      lastDisplayToggle = millis();

      if (currentMode == FREE_MODE)
      {
        metronome.start();
      }
      else
      {
        metronome.stop();
      }
    }
    else if (buttons.isRightLongPress() && currentMode == PATCH_MODE)
    {
      currentPatch = (currentPatch + 1) % storage.getCurrentNumPatches();
      showingPatchName = true;
      lastDisplayToggle = millis();
      metronome.setTempo(patches[currentPatch].tempo);
    }
    else if (buttons.wasLeftButtonPressed())
    {
      if (currentMode == PATCH_MODE)
      {
        currentPatch = (currentPatch - 1 + storage.getCurrentNumPatches()) % storage.getCurrentNumPatches();
        showingPatchName = true;
        lastDisplayToggle = millis();
        metronome.setTempo(patches[currentPatch].tempo);
      }
    }
    else if (buttons.wasRightButtonPressed())
    {
      if (currentMode == PATCH_MODE)
      {
        if (metronome.isRunning())
        {
          metronome.stop();
        }
        else
        {
          metronome.start();
        }
      }
      else
      {
        metronome.tap();
      }
    }

    display.update(currentMode, currentPatch, patches,
                   metronome.getTempo(),
                   showingPatchName,
                   wifiManager.isConnected(),
                   settings.liveGigMode);

    buttons.clearButtonStates();
  }

  handleDisplayToggle();
  checkDisplayTimeout();
  metronome.update(displayActive);
}