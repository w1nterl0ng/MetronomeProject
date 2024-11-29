#include "wifi_manager.h"
#include "storage.h"
#include <ArduinoJson.h>
#include <LittleFS.h>

WiFiManager::WiFiManager(Patch *patches, Settings &settings, Display &display) : server(80),
                                                                                 wifiConnected(false),
                                                                                 wifiAttempting(false),
                                                                                 wifiStartAttemptTime(0),
                                                                                 patches(patches),
                                                                                 settings(settings),
                                                                                 display(display)
{
}

String getContentType(String filename)
{
    if (filename.endsWith(".html"))
        return "text/html";
    if (filename.endsWith(".css"))
        return "text/css";
    if (filename.endsWith(".js"))
        return "application/javascript";
    return "text/plain";
}

void WiFiManager::begin()
{
    if (!LittleFS.begin())
    {
        Serial.println("Error mounting LittleFS");
        return;
    }

    Serial.print("Starting WiFi connection attempt...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiStartAttemptTime = millis();
    wifiAttempting = true;
    wifiConnected = false;

    setupServerRoutes();

    // Handle static files
    server.onNotFound([this]()
                      {
        String path = server.uri();
        if (path.endsWith("/")) path += "index.html";
        
        if (LittleFS.exists(path)) {
            File file = LittleFS.open(path, "r");
            server.streamFile(file, getContentType(path));
            file.close();
            return;
        }
        
        server.send(404, "text/plain", "File Not Found"); });
}

void WiFiManager::update()
{
    if (wifiAttempting)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            wifiConnected = true;
            wifiAttempting = false;
            Serial.println("\nWiFi Connected!");
            Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());

            server.begin();
            Serial.println("Web server started");
        }
        else if (millis() - wifiStartAttemptTime > WIFI_TIMEOUT)
        {
            wifiAttempting = false;
            wifiConnected = false;
            WiFi.disconnect(true);
            Serial.println("\nWiFi connection timed out. Running in offline mode.");
        }
    }

    if (wifiConnected)
    {
        server.handleClient();
    }
}

void WiFiManager::setupServerRoutes()
{
    // Get all patches
    server.on("/api/patches", HTTP_GET, [this]()
              {
        StaticJsonDocument<1024> doc;
        JsonArray array = doc.to<JsonArray>();
        
        for (int i = 0; i < storage.getCurrentNumPatches(); i++) {
            JsonObject patch = array.createNestedObject();
            patch["name"] = patches[i].name;
            patch["tempo"] = patches[i].tempo;
        }
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response); });

    // Update a patch
    server.on("/api/patches", HTTP_PUT, [this]()
              {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (!error) {
            int index = doc["index"] | -1;
            JsonObject patchObj = doc["patch"];
            
            if (index >= 0 && index < MAX_PATCHES) {
                strncpy(patches[index].name, patchObj["name"] | "", sizeof(patches[index].name));
                patches[index].tempo = patchObj["tempo"] | 120;
                storage.savePatches(patches, MAX_PATCHES);
                server.send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                server.send(400, "application/json", "{\"error\":\"Invalid patch index\"}");
            }
        } });

    // Get settings
    server.on("/api/settings", HTTP_GET, [this]()
              {
        StaticJsonDocument<200> doc;
        doc["liveGigMode"] = settings.liveGigMode;
        doc["brightness"] = settings.brightness;
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response); });

    // Update settings
    server.on("/api/settings", HTTP_POST, [this]()
              {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (!error) {
            settings.liveGigMode = doc["liveGigMode"] | false;
            settings.brightness = doc["brightness"] | 1;
            
            // Update the display brightness immediately
            display.setBrightness(settings.brightness);
            
            storage.saveSettings(settings);
            server.send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        } });
}
