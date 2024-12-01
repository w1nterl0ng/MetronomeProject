#include "wifi_manager.h"
#include "storage.h"
#include "debug.h"
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
        DEBUG_PRINTLN("Error mounting LittleFS");
        return;
    }

    DEBUG_PRINT("Starting WiFi connection attempt...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    wifiStartAttemptTime = millis();
    wifiAttempting = true;
    wifiConnected = false;

    setupServerRoutes();

    // Handle static files
    server.onNotFound([this]()
                      {
        String path = server.uri();
        DEBUG_PRINT("Requested path: ");
        DEBUG_PRINTLN(path);
        
        if (path.endsWith("/")) {
            path += "index.html";
            DEBUG_PRINTLN("Adding index.html to path");
        }
        
        DEBUG_PRINT("Looking for file: ");
        DEBUG_PRINTLN(path);
        
        if (LittleFS.exists(path)) {
            DEBUG_PRINTLN("File exists!");
            File file = LittleFS.open(path, "r");
            server.streamFile(file, getContentType(path));
            file.close();
            return;
        }
        
        DEBUG_PRINTLN("File not found!");
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
            DEBUG_PRINTLN("\nWiFi Connected!");
            DEBUG_PRINTF("IP address: %s\n", WiFi.localIP().toString().c_str());

            server.begin();
            DEBUG_PRINTLN("Web server started");
        }
        else if (millis() - wifiStartAttemptTime > WIFI_TIMEOUT)
        {
            wifiAttempting = false;
            wifiConnected = false;
            WiFi.disconnect(true);
            DEBUG_PRINTLN("\nWiFi connection timed out. Running in offline mode.");
        }
    }
    else if (!wifiConnected && (WiFi.status() != WL_CONNECTED))
    {
        DEBUG_PRINTLN("\nLost WiFi connection. Attempting to reconnect...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        wifiStartAttemptTime = millis();
        wifiAttempting = true;
    }

    if (wifiConnected)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            DEBUG_PRINTLN("WiFi connection lost!");
            wifiConnected = false;
        }
        else
        {
            server.handleClient();
        }
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

    // Create new patch
    server.on("/api/patches", HTTP_POST, [this]()
              {
        DEBUG_PRINTLN("POST /api/patches received");
        
        if (server.hasArg("plain")) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, server.arg("plain"));
            
            if (!error) {
                int currentCount = storage.getCurrentNumPatches();
                DEBUG_PRINTF("Current patch count before add: %d\n", currentCount);
                
                if (currentCount < MAX_PATCHES) {
                    const char* newName = doc["name"] | "";
                    int newTempo = doc["tempo"] | 120;
                    
                    DEBUG_PRINTF("Adding new patch: name='%s', tempo=%d at index %d\n", 
                                newName, newTempo, currentCount);
                    
                    strlcpy(patches[currentCount].name, newName, sizeof(patches[currentCount].name));
                    patches[currentCount].tempo = newTempo;
                    
                    storage.savePatchCount(currentCount + 1);
                    storage.savePatches(patches, MAX_PATCHES);
                    
                    DEBUG_PRINTF("New patch count: %d\n", storage.getCurrentNumPatches());
                    
                    server.send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    DEBUG_PRINTLN("Error: Maximum patches reached");
                    server.send(400, "application/json", "{\"error\":\"Maximum number of patches reached\"}");
                }
            } else {
                DEBUG_PRINTLN("Error: Invalid JSON");
                server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            }
        } });

    // Update patch
    server.on("/api/patches", HTTP_PUT, [this]()
              {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (!error) {
            int index = doc["index"] | -1;
            JsonObject patchObj = doc["patch"];
            
            if (index >= 0 && index < MAX_PATCHES) {
                strlcpy(patches[index].name, patchObj["name"] | "", sizeof(patches[index].name));
                patches[index].tempo = patchObj["tempo"] | 120;
                storage.savePatches(patches, MAX_PATCHES);
                server.send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                server.send(400, "application/json", "{\"error\":\"Invalid patch index\"}");
            }
        } });

    // Delete patch
    server.on("/api/patches", HTTP_DELETE, [this]()
              {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));

        if (!error) {
            int index = doc["index"] | -1;
            if (index >= 0 && index < storage.getCurrentNumPatches()) {
                int numPatches = storage.getCurrentNumPatches();
                
                for (int i = index; i < numPatches - 1; i++) {
                    strncpy(patches[i].name, patches[i + 1].name, sizeof(patches[i].name));
                    patches[i].tempo = patches[i + 1].tempo;
                }

                memset(patches[numPatches - 1].name, 0, sizeof(patches[numPatches - 1].name));
                patches[numPatches - 1].tempo = 120;

                storage.savePatchCount(numPatches - 1);
                storage.savePatches(patches, MAX_PATCHES);

                DEBUG_PRINTF("Deleted patch at index %d, new patch count: %d\n", 
                            index, storage.getCurrentNumPatches());
                
                server.send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                server.send(400, "application/json", "{\"error\":\"Invalid patch index\"}");
            }
        } });

    // Settings endpoints
    server.on("/api/settings", HTTP_GET, [this]()
              {
        StaticJsonDocument<200> doc;
        doc["brightness"] = settings.brightness;
        
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response); });

    server.on("/api/settings", HTTP_POST, [this]()
              {
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        
        if (!error) {
            settings.brightness = doc["brightness"] | 1;
            display.setBrightness(settings.brightness);
            storage.saveSettings(settings);
            server.send(200, "application/json", "{\"status\":\"success\"}");
        } });
}