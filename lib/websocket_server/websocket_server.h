#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "global_var.h"
#include "ble_multi_client.h"
#include <ESPmDNS.h>


/**
 * @brief Handles Wi-Fi events and updates the NeoPixel LED based on connection status.
 * 
 * @param event The Wi-Fi event type.
 */
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("WiFi Connected!");
            wifiConnected = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi Disconnected!");
            wifiConnected = false;
            // Red light when Wi-Fi is not connected
            pixels.setPixelColor(0, Adafruit_NeoPixel::Color(255, 0, 0)); // Red
            pixels.show();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            wifiConnected = true;
            Serial.print("WiFi Connected, IP: ");
            Serial.println(WiFi.localIP());
            // Turn off the LED when connected
            pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0)); // None
            pixels.show();
            pixels.clear();
            break;
        default:
            break;
    }
}


/**
 * @brief Class representing an ESP32 WebSocket server with BLE integration.
 */
class ESP32WebSocketServer {
public:
    /**
     * @brief Constructor for the WebSocket server.
     * 
     * @param ssid The Wi-Fi SSID.
     * @param password The Wi-Fi password.
     */
    ESP32WebSocketServer(const char* ssid, const char* password)
    : ssid(ssid), password(password), server(80), webSocket(81) {}

    /**
     * @brief Initializes the WebSocket server, BLE client, and other components.
     */
    void begin() {
        pixels.begin();
        pixels.clear();
        Serial.begin(115200);
        WiFi.onEvent(WiFiEvent);
        WiFi.begin(ssid, password);
        while (WiFiClass::status() != WL_CONNECTED)
        {
            delay(500);
            Serial.println("Connecting to WiFi...");
        }

#if USE_BLYNK == false

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
    

        // Serve HTML from SPIFFS
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)    {
            if (SPIFFS.exists("/index.html")) 
                request->send(SPIFFS, "/index.html", "text/html");
            else 
            {
                Serial.println("[ERROR] index.html not found in SPIFFS!");
                request->send(404, "text/plain", "File Not Found");
            }
        });
        // Set hostname
        if (MDNS.begin("ac-control"))   Serial.println("mDNS responder started");
        // Start Websocket
        webSocket.begin();
        webSocket.onEvent([this](uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
            this->onWebSocketEvent(num, type, payload, length);
        });
        loadServerData(webSocket);
        delay(1000);
        server.begin();
#endif
        bleClient.init();
        bleClient.startScanning();
    }

    /**
     * @brief Main loop for handling WebSocket and BLE events.
     */
    void loop() {
#if USE_BLYNK == false
        webSocket.loop();
#endif
        ble_loop(webSocket);
        delay(50);  // Required for ESP32 to run properly
    }

private:
    const char* ssid;  // Wi-Fi SSID
    const char* password;  // Wi-Fi password
    AsyncWebServer server;  // HTTP server
    WebSocketsServer webSocket;  // WebSocket server
    /**
     * @brief Toggles the state of a damper or AC based on WebSocket messages.
     * 
     * @param message The WebSocket message.
     */
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
            sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.getClientForDamper(0), std::string(newState.c_str()));
            // Save last value to NVS
            saveState("ac_state", statusMessage);
        }
    
    }

    /**
     * @brief Sets the power level of a damper or AC based on WebSocket messages.
     * 
     * @param message The WebSocket message.
     */
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
            sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.getClientForDamper(0)/*bleClient.acClient*/, std::string(power_state.c_str()));
            // Save last value to NVS
            saveState("ac_power", statusMessage);
        }
    }

    /**
     * @brief Sets the AC mode (e.g., heat or cold) based on WebSocket messages.
     * 
     * @param message The WebSocket message.
     */
    void acMode(const String& message) { // Heat or cold
        NimBLEUUID CHARACTERISTIC_UUID = NimBLEUUID(MODE_UUID);
        String mode = message.substring(12);
        String statusMessage = "ac_mode: " + mode;
        webSocket.broadcastTXT(statusMessage);
        // Send to BLE Peripheral
        sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.getClientForDamper(0) /*bleClient.acClient*/, std::string(mode.c_str()));
        // Save last value to NVS
        saveState("ac_mode", statusMessage);
    }

    /**
     * @brief Sets the AC temperature based on WebSocket messages.
     * 
     * @param message The WebSocket message.
     */
    void acTemp(const String& message) {
        NimBLEUUID CHARACTERISTIC_UUID = NimBLEUUID(TEMP_UUID);
        String temp = message.substring(12);
        String statusMessage = "ac_temp: " + temp;
        webSocket.broadcastTXT(statusMessage);
        // Send to BLE Peripheral
        sendDataToPeripheral(CHARACTERISTIC_UUID, bleClient.getClientForDamper(0)/*bleClient.acClient*/, std::string(temp.c_str()));
        // Save last value to NVS
        saveState("ac_temp", statusMessage);
    }

    /**
     * @brief Handles incoming WebSocket messages.
     * 
     * @param num The WebSocket client number.
     * @param payload The message payload.
     * @param length The length of the payload.
     */
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

    /**
     * @brief Handles WebSocket events such as connection and message reception.
     * 
     * @param num The WebSocket client number.
     * @param type The type of WebSocket event.
     * @param payload The event payload.
     * @param length The length of the payload.
     */
    void onWebSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
        if (type == WStype_CONNECTED)   loadServerData(webSocket);
        if (type == WStype_TEXT)   handleWebSocketMessage(num, payload, length);

    } 
};

#endif // WEBSOCKET_SERVER_H

