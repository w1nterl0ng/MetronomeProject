#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "config.h"
#include "types.h"
#include "display.h"
#include "buttons.h"
#include "storage.h"
#include "wifi_manager.h"
#include "metronome.h"

Display display;
Buttons buttons;
Metronome metronome;
Settings settings;
Patch patches[MAX_PATCHES];
WiFiManager wifiManager(patches, settings, display); // Initialize with references

// Global state
Mode currentMode = PATCH_MODE;
int currentPatch = 0;
bool showingPatchName = true;
unsigned long lastActivityTime = 0;
bool displayActive = true;
unsigned long lastDisplayToggle = 0;

void updateActivity()
{
  lastActivityTime = millis();
  displayActive = true;
}

bool isLiveGigMode()
{
  static bool lastState = false;
  bool currentState = digitalRead(LIVE_GIG_PIN) == LOW;

  // If state changed, record the time
  if (currentState != lastState)
  {
    DEBUG_PRINTF("Live Gig Mode changed to: %s\n", currentState ? "ON" : "OFF");
    lastState = currentState;

    if (currentState)
    {
      // Force a clean state when entering live gig mode
      displayActive = true;
      metronome.start();
      updateActivity();
    }
    else
    {
      // Clean up when exiting live gig mode
      metronome.stop();
    }
  }

  return currentState;
}

void checkDisplayTimeout()
{
  if (isLiveGigMode() && displayActive)
  {
    if (millis() - lastActivityTime > LIVE_GIG_TIMEOUT)
    {
      displayActive = false;
      DEBUG_PRINTLN("Display timeout - turning off");
    }
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
                     isLiveGigMode());
    }
  }
}

void setup()
{
  Serial.begin(115200);
  DEBUG_PRINTLN("\nStarting Metronome...");

  // Add emergency recovery check
  pinMode(LIVE_GIG_PIN, INPUT_PULLUP);
  pinMode(LEFT_SWITCH_PIN, INPUT_PULLUP);
  pinMode(RIGHT_SWITCH_PIN, INPUT_PULLUP);

  // If both buttons are held during power-up, reset everything
  if (digitalRead(LEFT_SWITCH_PIN) == LOW && digitalRead(RIGHT_SWITCH_PIN) == LOW)
  {
    DEBUG_PRINTLN("Emergency reset triggered!");
    EEPROM.begin(512);
    for (int i = 0; i < 512; i++)
    {
      EEPROM.write(i, 0xFF);
    }
    EEPROM.commit();
    DEBUG_PRINTLN("EEPROM cleared");
    delay(1000);
    ESP.restart();
  }

  Wire.begin();

  // Initialize WiFi first
  wifiManager.begin();

  // Then other peripherals
  pinMode(LIVE_GIG_PIN, INPUT_PULLUP);
  display.begin();
  buttons.begin();
  storage.begin();
  metronome.begin();

  // Add watchdog
  ESP.wdtEnable(WDTO_8S);

  settings = storage.loadSettings();
  storage.loadPatches(patches, MAX_PATCHES);

  display.setBrightness(settings.brightness);
  metronome.setLiveGigMode(isLiveGigMode());
  metronome.setTempo(patches[currentPatch].tempo);

  updateActivity();
  lastDisplayToggle = millis();

  display.update(currentMode, currentPatch, patches,
                 metronome.getTempo(),
                 showingPatchName,
                 wifiManager.isConnected(),
                 isLiveGigMode());
}

void loop()
{
  // Reset watchdog timer
  ESP.wdtFeed();

  wifiManager.update();

  // Update live gig mode from switch
  metronome.setLiveGigMode(isLiveGigMode());

  if (buttons.update())
  {
    updateActivity(); // Reset activity timer

    if (buttons.isLeftLongPress())
    {
      if (!isLiveGigMode())
      { // Only allow mode change if not in live gig mode
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
    }
    else if (buttons.isRightLongPress() && currentMode == PATCH_MODE)
    {
      if (isLiveGigMode())
      {
        updateActivity(); // Reset timeout
        DEBUG_PRINTLN("Live Gig timeout reset");
      }
      else
      {
        currentPatch = (currentPatch + 1) % storage.getCurrentNumPatches();
        showingPatchName = true;
        lastDisplayToggle = millis();
        metronome.setTempo(patches[currentPatch].tempo);
      }
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
        if (isLiveGigMode())
        {
          currentPatch = (currentPatch + 1) % storage.getCurrentNumPatches();
          showingPatchName = true;
          lastDisplayToggle = millis();
          metronome.setTempo(patches[currentPatch].tempo);
        }
        else
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
                   isLiveGigMode());

    buttons.clearButtonStates();
  }

  handleDisplayToggle();
  checkDisplayTimeout();
  metronome.update(displayActive);
}