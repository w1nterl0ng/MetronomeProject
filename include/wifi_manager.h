#pragma once

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "config.h"
#include "types.h"

class WiFiManager
{
public:
    WiFiManager(Patch *patches, Settings &settings);
    void begin();
    void update();
    bool isConnected() const { return wifiConnected; }
    ESP8266WebServer &getServer() { return server; }

private:
    ESP8266WebServer server;
    bool wifiConnected;
    bool wifiAttempting;
    unsigned long wifiStartAttemptTime;

    Patch *patches;
    Settings &settings;

    void setupServerRoutes();
};

extern WiFiManager wifiManager;