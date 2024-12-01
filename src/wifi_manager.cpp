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
        Serial.print("Requested path: ");
        Serial.println(path);
        
        if (path.endsWith("/")) {
            path += "index.html";
            Serial.println("Adding index.html to path");
        }
        
        Serial.print("Looking for file: ");
        Serial.println(path);
        
        if (LittleFS.exists(path)) {
            Serial.println("File exists!");
            File file = LittleFS.open(path, "r");
            server.streamFile(file, getContentType(path));
            file.close();
            return;
        }
        
        Serial.println("File not found!");
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
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());

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
    else if (!wifiConnected && (WiFi.status() != WL_CONNECTED))
    {
        Serial.println("\nLost WiFi connection. Attempting to reconnect...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        wifiStartAttemptTime = millis();
        wifiAttempting = true;
    }

    if (wifiConnected)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi connection lost!");
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
        Serial.println("POST /api/patches received");
        
        if (server.hasArg("plain")) {
            StaticJsonDocument<200> doc;
            DeserializationError error = deserializeJson(doc, server.arg("plain"));
            
            if (!error) {
                int currentCount = storage.getCurrentNumPatches();
                Serial.printf("Current patch count before add: %d\n", currentCount);
                
                if (currentCount < MAX_PATCHES) {
                    const char* newName = doc["name"] | "";
                    int newTempo = doc["tempo"] | 120;
                    
                    Serial.printf("Adding new patch: name='%s', tempo=%d at index %d\n", 
                                newName, newTempo, currentCount);
                    
                    strlcpy(patches[currentCount].name, newName, sizeof(patches[currentCount].name));
                    patches[currentCount].tempo = newTempo;
                    
                    storage.savePatchCount(currentCount + 1);
                    storage.savePatches(patches, MAX_PATCHES);
                    
                    Serial.printf("New patch count: %d\n", storage.getCurrentNumPatches());
                    
                    server.send(200, "application/json", "{\"status\":\"success\"}");
                } else {
                    Serial.println("Error: Maximum patches reached");
                    server.send(400, "application/json", "{\"error\":\"Maximum number of patches reached\"}");
                }
            } else {
                Serial.println("Error: Invalid JSON");
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

                Serial.printf("Deleted patch at index %d, new patch count: %d\n", 
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