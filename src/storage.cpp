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

Settings Storage::getDefaultSettings()
{
    Settings settings;
    settings.brightness = 8; // Medium brightness
    settings.checksum = SETTINGS_CHECKSUM;
    return settings;
}

Settings Storage::loadSettings()
{
    Settings settings;
    EEPROM.get(SETTINGS_ADDR, settings);

    if (settings.checksum != SETTINGS_CHECKSUM)
    {
        Serial.println("Invalid settings, initializing defaults");
        settings = getDefaultSettings();
        saveSettings(settings);
    }

    Serial.printf("Loaded settings - Brightness: %d\n", settings.brightness);

    return settings;
}

void Storage::saveSettings(const Settings &settings)
{
    EEPROM.put(SETTINGS_ADDR, settings);
    EEPROM.commit();
}

bool Storage::validatePatch(const Patch &patch)
{
    for (int i = 0; i < 4; i++)
    {
        if (patch.name[i] < 32 || patch.name[i] > 126)
        {
            return false;
        }
    }

    if (patch.tempo < 40 || patch.tempo > 240)
    {
        return false;
    }

    return true;
}

void Storage::initializeDefaultPatches(Patch *patches)
{
    strncpy(patches[0].name, "STOK", 5);
    patches[0].tempo = 110;

    strncpy(patches[1].name, "CRY ", 5);
    patches[1].tempo = 125;

    strncpy(patches[2].name, "SHIP", 5);
    patches[2].tempo = 118;

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
    EEPROM.get(PATCHES_ADDR, patches[0]);

    if (!validatePatch(patches[0]))
    {
        Serial.println("Invalid patches found, initializing defaults");
        initializeDefaultPatches(patches);
        return;
    }

    for (int i = 1; i < maxPatches; i++)
    {
        EEPROM.get(PATCHES_ADDR + (i * sizeof(Patch)), patches[i]);
    }

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
    Serial.println("Storage: Saving patches to EEPROM");
    for (int i = 0; i < maxPatches; i++)
    {
        EEPROM.put(PATCHES_ADDR + (i * sizeof(Patch)), patches[i]);
    }
    if (EEPROM.commit())
    {
        Serial.println("Storage: EEPROM commit successful");
    }
    else
    {
        Serial.println("Storage: EEPROM commit failed!");
    }
}

void Storage::savePatchCount(int count)
{
    Serial.printf("Storage: Updating patch count from %d to %d\n", numPatches, count);
    numPatches = count;
}

int Storage::getCurrentNumPatches() const
{
    Serial.printf("Storage: Current patch count is %d\n", numPatches);
    return numPatches;
}