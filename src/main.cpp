#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "config.h"
#include "types.h"
#include "display.h"
#include "buttons.h"
#include "storage.h"
#include "metronome.h"

Display display;
Buttons buttons;
Metronome metronome;

// Global state
Mode currentMode = PATCH_MODE;
int currentPatch = 0;
bool showingPatchName = true;
Settings settings;
Patch patches[MAX_PATCHES];
unsigned long lastActivityTime = 0;
unsigned long lastDisplayToggle = 0; // Added for display rotation
bool displayActive = true;

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
                     showingPatchName, false,
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

  settings = storage.loadSettings();
  storage.loadPatches(patches, MAX_PATCHES);

  display.setBrightness(settings.brightness);
  metronome.setLiveGigMode(settings.liveGigMode);
  metronome.setTempo(patches[currentPatch].tempo);

  updateActivity();
  lastDisplayToggle = millis(); // Initialize display toggle timer

  display.update(currentMode, currentPatch, patches,
                 metronome.getTempo(),
                 showingPatchName, false,
                 settings.liveGigMode);
}

// Add debug helper function
void printState(const char *event)
{
  Serial.printf("\n[DEBUG] %s\n", event);
  Serial.printf("Mode: %s\n", currentMode == PATCH_MODE ? "PATCH_MODE" : "FREE_MODE");
  Serial.printf("Running: %s\n", metronome.isRunning() ? "true" : "false");
  Serial.printf("Tap Mode: %s\n", metronome.isInTapMode() ? "true" : "false");
  Serial.printf("Tempo: %d\n", metronome.getTempo());
  Serial.println("------------------------");
}

void loop()
{
  if (buttons.update())
  {
    updateActivity();

    // Handle Long Presses First
    if (buttons.isLeftLongPress())
    {
      Serial.println("Left Button Long Press Detected");
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
      printState("After Mode Change");
    }
    else if (buttons.isRightLongPress() && currentMode == PATCH_MODE)
    {
      Serial.println("Right Button Long Press Detected");
      currentPatch = (currentPatch + 1) % storage.getCurrentNumPatches();
      showingPatchName = true;
      lastDisplayToggle = millis();
      metronome.setTempo(patches[currentPatch].tempo);
      printState("After Next Patch");
    }
    // Handle Short Presses
    else if (buttons.wasLeftButtonPressed())
    {
      Serial.println("Left Button Short Press Detected");
      if (currentMode == PATCH_MODE)
      {
        currentPatch = (currentPatch - 1 + storage.getCurrentNumPatches()) % storage.getCurrentNumPatches();
        showingPatchName = true;
        lastDisplayToggle = millis();
        metronome.setTempo(patches[currentPatch].tempo);
        printState("After Left Button Short Press - Patch Change");
      }
    }
    else if (buttons.wasRightButtonPressed())
    {
      Serial.println("Right Button Short Press Detected");
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
        printState("After Right Button Short Press - Patch Mode Toggle");
      }
      else
      { // FREE_MODE
        Serial.println("Attempting Tap in FREE_MODE");
        metronome.tap();
        printState("After Tap Tempo");
      }
    }

    display.update(currentMode, currentPatch, patches,
                   metronome.getTempo(),
                   showingPatchName, false,
                   settings.liveGigMode);

    buttons.clearButtonStates();
  }
  handleDisplayToggle();
  checkDisplayTimeout();
  metronome.update(displayActive);
}