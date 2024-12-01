#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include "types.h"
#include "config.h"

class Storage
{
public:
    Storage();
    void begin();

    // Settings management
    Settings loadSettings();
    void saveSettings(const Settings &settings);
    Settings getDefaultSettings();

    // Patch management
    void loadPatches(Patch *patches, int maxPatches);
    void savePatches(const Patch *patches, int maxPatches);
    int getCurrentNumPatches() const;
    void savePatchCount(int count);

private:
    int numPatches;
    bool validatePatch(const Patch &patch);
    void initializeDefaultPatches(Patch *patches);
};

extern Storage storage;