#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H
#include <SPIFFS.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <ESPAsyncWebServer.h>
#include "ble_multi_client.h"
#include <Adafruit_NeoPixel.h> 
#include <nvs.h>
#include <nvs_flash.h>

#define PIN 48
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//const char* hostname = "ESP_SERVER";

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("WiFi Connected!");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi Disconnected!");
            // Red light when Wi-Fi Not connected
            pixels.setPixelColor(0, Adafruit_NeoPixel::Color(255, 0, 0)); // Red
            pixels.show();
            // Your action when Wi-Fi disconnects
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("WiFi Connected, IP: ");
            Serial.println(WiFi.localIP());
            pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0)); // None
            pixels.show();
            pixels.clear();
            break;
        default:
            break;
    }
}

void saveState(const String& key, const String& state) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        Serial.println("Error opening NVS handle!");
        return;
    }
    err = nvs_set_str(nvs_handle, key.c_str(), state.c_str());
    if (err != ESP_OK) {
        Serial.println("Error setting NVS value!");
    }
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        Serial.println("Error committing NVS value!");
    }
    nvs_close(nvs_handle);
}

String loadState(const String& key) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        Serial.println("Error opening NVS handle!");
        return "";
    }
    size_t required_size;
    err = nvs_get_str(nvs_handle, key.c_str(), NULL, &required_size);
    if (err != ESP_OK) {
        Serial.println("Error getting NVS value size!");
        nvs_close(nvs_handle);
        return "";
    }
    char* value = (char*)malloc(required_size);
    err = nvs_get_str(nvs_handle, key.c_str(), value, &required_size);
    if (err != ESP_OK) {
        Serial.println("Error getting NVS value!");
        free(value);
        nvs_close(nvs_handle);
        return "";
    }
    String result = String(value);
    free(value);
    nvs_close(nvs_handle);
    return result;
}

void loadServerData(WebSocketsServer& webSocket)
{
    // Load and apply saved states
    for (int i = 0; i < 3; ++i)
    {
        String damperState = loadState("damper" + String(i + 1) + "_state");
        if (damperState != "")  webSocket.broadcastTXT(damperState);

        String damperPower = loadState("damper" + String(i + 1) + "_power");
        if (damperPower != "") webSocket.broadcastTXT(damperPower);
        Serial.println("Loaded states from NVS:");
        Serial.println(damperState);
        Serial.println(damperPower);
    }
    String acState = loadState("ac_state");
    if (acState != "")  webSocket.broadcastTXT(acState);

    String acPower = loadState("ac_power");
    if (acPower != "")   webSocket.broadcastTXT(acPower);

    String acMode = loadState("ac_mode");
    if (acMode != "")  webSocket.broadcastTXT(acMode);

    String acTemp = loadState("ac_temp");
    if (acTemp != "")   webSocket.broadcastTXT(acTemp);

    Serial.println("Loaded states from NVS:");
    Serial.println(acTemp);
    Serial.println(acState);
    Serial.println(acPower);
    Serial.println(acMode);
}

class ESP32WebSocketServer {
public:

    ESP32WebSocketServer(const char* ssid, const char* password)
    : ssid(ssid), password(password), server(80), webSocket(81) {}
    void begin() {
        pixels.begin();
        pixels.clear();
        Serial.begin(115200);
        Serial.println("Initializing WebSocket & BLE...");
    
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);

        if (!SPIFFS.begin(false))
        { // Try mounting first
            Serial.println("SPIFFS mount failed! Formatting...");
            SPIFFS.format(); // Format manually only if mount fails
            SPIFFS.begin(true); // Try mounting again after format
        }
        else    Serial.println("SPIFFS mounted successfully!");
        
        WiFi.onEvent(WiFiEvent);
        WiFi.begin(ssid, password);

        while (WiFiClass::status() != WL_CONNECTED) {
            delay(500);
            Serial.println("Connecting to WiFi...");
        }

        // Serve HTML from SPIFFS
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (SPIFFS.exists("/index.html")) {
                request->send(SPIFFS, "/index.html", "text/html");
            } else {
                Serial.println("[ERROR] index.html not found in SPIFFS!");
                request->send(404, "text/plain", "File Not Found");
            }
        });

        // Start Websocket
        webSocket.begin();
        webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
            this->onWebSocketEvent(num, type, payload, length);
        });
        loadServerData(webSocket);
        delay(1000);
        server.begin();
        bleClient.init();
        bleClient.startScanning();
    }

    void loop() {
        webSocket.loop();
        if(bleClient.doConnect)
        {
            bleClient.connectToDevice();
            bleClient.doConnect = false;
            bleClient.startScanning();
        }
        if (bleClient.notified)
        {
            std::string statusMessage;

            if (bleClient.notifty_index != -1 && bleClient.notifty_index < 4)
            {
                if (bleClient.notifty_index == 0)
                    statusMessage = "voltage_ac:" + bleClient.per_voltage;
                else
                    statusMessage = "voltage_damper" + std::to_string(bleClient.notifty_index) + ":" + bleClient.per_voltage;
                String statusMessageStr = String(statusMessage.c_str());
                webSocket.broadcastTXT(statusMessageStr);
            }

            bleClient.notified = false;
            bleClient.notifty_index = -1;
        }
        delay(50);  // Required for ESP32 to run properly
    }

    void sendDataToPeripheral(const NimBLEUUID CHARACTERISTIC_UUID, NimBLEClient* pClient, const std::string& value = "Hello") const {
        if (!pClient) {
            Serial.println("Client NOT connected!");
            return;
        }
        NimBLERemoteService* pService = pClient->getService(SERVICE_UUID);
        if (pService) {
            NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(CHARACTERISTIC_UUID);
            if (pCharacteristic)
            {
                // Replace with the data you want to send
                pCharacteristic->writeValue(value);
                Serial.print("Data sent to peripheral! UUID: ");
                Serial.print(CHARACTERISTIC_UUID.toString().c_str());
                Serial.print("! Value ");
                Serial.println(value.c_str());
            }
            else
                Serial.println("Characteristic NOT found!");
        } else
            Serial.println("Service NOT found!");

    }

private:
    const char* ssid;
    const char* password;
    AsyncWebServer server;
    WebSocketsServer webSocket;
    BLEClientMulti bleClient;
    
    void toggleButton(const String& message) {
        NimBLEUUID CHARACTERISTIC_UUID = NimBLEUUID(STATE_UUID);

        static bool damperStates[3] = {false};  // A
        static bool acState = false;
        if (message.startsWith("toggle_damper")) {
            // Extract damper number from the message
            int damperIndex = message.substring(13).toInt() - 1;  // Get damper number (e.g., "1" for damper1)
    
            if (damperIndex >= 0 && damperIndex < sizeof(damperStates) / sizeof(damperStates[0])) {
                damperStates[damperIndex] = !damperStates[damperIndex];  // Toggle state
                String newState = damperStates[damperIndex] ? "on" : "off";
    
                // Send update to the WebSocket clients
                String statusMessage = "status_damper" + String(damperIndex + 1) + ":" + newState;
                webSocket.broadcastTXT(statusMessage);
                Serial.println(statusMessage.c_str());
                NimBLEClient* pClient = bleClient.getClientForDamper(damperIndex+1);
                sendDataToPeripheral(CHARACTERISTIC_UUID, pClient, std::string(newState.c_str()));
                // Save last value to NVS
                saveState("damper" + String(damperIndex + 1) + "_state", statusMessage);

            }
        }
        else if  (message.startsWith("toggle_ac"))
        {
            acState = !acState;
            String newState = acState? "on" : "off";
            // Send update to the WebSocket clients
            String statusMessage = "status_ac:" + newState;
            webSocket.broadcastTXT(statusMessage);
            // Send to BLE Peripheral
            sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.acClient, std::string(newState.c_str()));
            // Save last value to NVS
            saveState("ac_state", statusMessage);
        }
    
    }
    void powerButton(const String& message) {
        // Low, Medium, High, Auto
        // Send message to WebSocket clients
        // Dynamic approach using an array for damper states
        NimBLEUUID CHARACTERISTIC_UUID = NimBLEUUID(VENT_SPEED_UUID);

        if (message.startsWith("power_damper")) {
            // Extract damper number from the message
            int damperIndex = message.substring(12).toInt() - 1;  // Get damper number (e.g., "1" for damper1)
            String power_state = message.substring(message.length() - 6);
            // Send update to the WebSocket clients
            String statusMessage = "power_damper" + String(damperIndex + 1) + ":" + power_state;
            webSocket.broadcastTXT(statusMessage);
            NimBLEClient* pClient = bleClient.getClientForDamper(damperIndex+1);
            sendDataToPeripheral(CHARACTERISTIC_UUID, pClient, std::string(power_state.c_str()));
            // Save last value to NVS
            saveState("damper" + String(damperIndex + 1) + "_power", statusMessage);
        }
        else if  (message.startsWith("power_ac"))
        {
            String power_state = message.substring(message.length() - 6);
            // Send update to the WebSocket clients
            String statusMessage = "power_ac: " + power_state;
            webSocket.broadcastTXT(statusMessage);
            // Send to BLE Peripheral
            sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.acClient, std::string(power_state.c_str()));
            // Save last value to NVS
            saveState("ac_power", statusMessage);
        }
    }
    void acMode(const String& message)
    { // Heat or cold
        NimBLEUUID CHARACTERISTIC_UUID = NimBLEUUID(MODE_UUID);
        String mode = message.substring(12);
        String statusMessage = "ac_mode: " + mode;
        webSocket.broadcastTXT(statusMessage);
        // Send to BLE Peripheral
        sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.acClient, std::string(mode.c_str()));
        // Save last value to NVS
        saveState("ac_mode", statusMessage);
    }
    void acTemp(const String& message)
    {
        NimBLEUUID CHARACTERISTIC_UUID = NimBLEUUID(TEMP_UUID);
        String temp = message.substring(12);
        String statusMessage = "ac_temp: " + temp;
        webSocket.broadcastTXT(statusMessage);
        // Send to BLE Peripheral
        sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.acClient, std::string(temp.c_str()));
        // Save last value to NVS
        saveState("ac_temp", statusMessage);
    }
    void handleWebSocketMessage(uint8_t num, uint8_t* payload, size_t length) {
        String message = String((char*)payload);
        Serial.println(message.c_str());
        if (message.startsWith("toggle"))  // Check if message is a toggle message
            toggleButton(message);
        else if (message.startsWith("power"))  // Check if message is a power message
            powerButton(message);
        else if (message.startsWith("set_ac_mode"))
            acMode(message);
        else if (message.startsWith("set_ac_temp"))
            acTemp(message);
        else
            Serial.println("Unknown message received!");       
    } 
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (type == WStype_CONNECTED)   loadServerData(webSocket);
        if (type == WStype_TEXT)   handleWebSocketMessage(num, payload, length);

    } 
};

#endif // WEBSOCKET_SERVER_H
