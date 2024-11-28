#include "storage.h"

Storage storage;

Storage::Storage() : numPatches(3)
{
}

void Storage::begin()
{
    EEPROM.begin(512);
    Serial.println("Storage system initialized");
}

Settings Storage::loadSettings()
{
    // TEMPORARY: Force reset settings
    Settings settings = getDefaultSettings();
    saveSettings(settings);

    Serial.printf("Settings reset - Live Gig Mode: %s, Brightness: %d\n",
                  settings.liveGigMode ? "true" : "false",
                  settings.brightness);

    return settings;
}

Settings Storage::getDefaultSettings()
{
    Settings settings;
    settings.liveGigMode = false; // Explicitly false
    settings.brightness = 0;      // Medium brightness
    settings.checksum = SETTINGS_CHECKSUM;
    return settings;
}

void Storage::saveSettings(const Settings &settings)
{
    EEPROM.put(SETTINGS_ADDR, settings);
    EEPROM.commit();
}

bool Storage::validatePatch(const Patch &patch)
{
    // Check if patch name contains valid ASCII characters
    for (int i = 0; i < 4; i++)
    {
        if (patch.name[i] < 32 || patch.name[i] > 126)
        {
            return false;
        }
    }

    // Check if tempo is within valid range
    if (patch.tempo < 40 || patch.tempo > 240)
    {
        return false;
    }

    return true;
}

void Storage::initializeDefaultPatches(Patch *patches)
{
    // Initialize default patches
    strncpy(patches[0].name, "STOK", 5);
    patches[0].tempo = 110;

    strncpy(patches[1].name, "CRY ", 5);
    patches[1].tempo = 125;

    strncpy(patches[2].name, "SHIP", 5);
    patches[2].tempo = 118;

    // Clear remaining patches
    for (int i = 3; i < MAX_PATCHES; i++)
    {
        patches[i].name[0] = '\0';
        patches[i].tempo = 120;
    }

    numPatches = 3;
    savePatches(patches, MAX_PATCHES);
}

void Storage::loadPatches(Patch *patches, int maxPatches)
{
    EEPROM.get(PATCHES_ADDR, patches[0]); // Load first patch to check validity

    if (!validatePatch(patches[0]))
    {
        Serial.println("Invalid patches found, initializing defaults");
        initializeDefaultPatches(patches);
        return;
    }

    // Load the rest of the patches
    for (int i = 1; i < maxPatches; i++)
    {
        EEPROM.get(PATCHES_ADDR + (i * sizeof(Patch)), patches[i]);
    }

    // Count number of valid patches
    numPatches = 0;
    for (int i = 0; i < maxPatches; i++)
    {
        if (patches[i].name[0] != '\0')
        {
            numPatches++;
        }
    }

    Serial.printf("Loaded %d patches\n", numPatches);
}

void Storage::savePatches(const Patch *patches, int maxPatches)
{
    for (int i = 0; i < maxPatches; i++)
    {
        EEPROM.put(PATCHES_ADDR + (i * sizeof(Patch)), patches[i]);
    }
    EEPROM.commit();
    Serial.println("Patches saved to EEPROM");
}