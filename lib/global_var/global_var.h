#ifndef GLOBALVAR_H
#define GLOBALVAR_H

#define USE_BLYNK true

// WiFi credentials
char ssid[] = "SSID_NAME";
char pass[] = "WIFI_PASS";
// Blynk credentials
#define BLYNK_TEMPLATE_ID "BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "AC"
#define BLYNK_AUTH_TOKEN "BLYNK_AUTH_TOKEN"

#include "ble_multi_client.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebSocketsServer.h>
#include <Adafruit_NeoPixel.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <BlynkSimpleEsp32.h>
#include <cstdlib>  // for atof

// Virtual Pins
#define VOLTAGE_START_PIN 7
// Pin and pixel configuration for NeoPixel LED
#define PIN 48
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// BLE client for multiple devices
BLEClientMulti bleClient;
// Global variable to track Wi-Fi connection status
bool wifiConnected = false;

// BLE Handler
/**
 * @brief Updates the NeoPixel LED based on BLE connection status.
 *
 * @param connected True if BLE is connected, false otherwise.
 */
void BLE_connected(bool connected) {
    if (connected) {
        // Turn off the LED when BLE is connected
        pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 0)); // None
        pixels.show();
        pixels.clear();
    } else {
        // Blue light when BLE is not connected
        pixels.setPixelColor(0, Adafruit_NeoPixel::Color(0, 0, 255)); // Blue
        pixels.show();
    }
}

/**
 * @brief Sends data to a BLE peripheral.
 *
 * @param CHARACTERISTIC_UUID The UUID of the BLE characteristic.
 * @param pClient The BLE client instance.
 * @param value The value to send.
 */
void sendDataToPeripheral(const NimBLEUUID CHARACTERISTIC_UUID, NimBLEClient* pClient, const std::string& value = "Hello") {
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

/**
 * @brief Handles BLE notifications and broadcasts them to WebSocket clients.
 */
void ble_notified(WebSocketsServer& webSocket) {
    std::string statusMessage;
    if (bleClient.notifty_index != -1 && bleClient.notifty_index < 4)
    {
#if USE_BLYNK == true
        Blynk.virtualWrite(VOLTAGE_START_PIN + bleClient.notifty_index , atof(bleClient.per_voltage.c_str()));
#else
        if (bleClient.notifty_index == 0)   statusMessage = "voltage_ac:" + bleClient.per_voltage;
        else    statusMessage = "voltage_damper" + std::to_string(bleClient.notifty_index) + ":" + bleClient.per_voltage;
        String statusMessageStr = String(statusMessage.c_str());
        webSocket.broadcastTXT(statusMessageStr);
#endif
    }
    bleClient.notified = false;
    bleClient.notifty_index = -1;
}
/**
    * @brief Handles BLE connection and notification events.
    */
void ble_loop(WebSocketsServer& webSocket) {
    if(bleClient.doConnect)
    {
        bleClient.connectToDevice();
        bleClient.doConnect = false;
        bleClient.startScanning();
    }
    if (bleClient.notified) ble_notified(webSocket);
    if (wifiConnected)
    {
        if (!bleClient.isConnected())   BLE_connected(false);
        else if (bleClient.isConnected())  BLE_connected(true);
    }
}


// NVS (Non-Volatile Storage) functions
/**
 * @brief Saves a key-value pair to NVS (Non-Volatile Storage).
 *
 * @param key The key to save.
 * @param state The value to save.
 */
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

/**
 * @brief Loads a value from NVS based on the given key.
 *
 * @param key The key to retrieve the value for.
 * @return The value associated with the key, or an empty string if not found.
 */
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

/**
 * @brief Loads saved states from NVS and broadcasts them to WebSocket clients.
 *
 * @param webSocket The WebSocket server instance.
 */
void loadServerData(WebSocketsServer& webSocket) {
    // Load and apply saved states for dampers and AC
    for (int i = 0; i < 3; ++i) {
        String damperState = loadState("damper" + String(i + 1) + "_state");
        if (damperState != "") webSocket.broadcastTXT(damperState);

        String damperPower = loadState("damper" + String(i + 1) + "_power");
        if (damperPower != "") webSocket.broadcastTXT(damperPower);

        Serial.println("Loaded states from NVS:");
        Serial.println(damperState);
        Serial.println(damperPower);
    }

    String acState = loadState("ac_state");
    if (acState != "") webSocket.broadcastTXT(acState);

    String acPower = loadState("ac_power");
    if (acPower != "") webSocket.broadcastTXT(acPower);

    String acMode = loadState("ac_mode");
    if (acMode != "") webSocket.broadcastTXT(acMode);

    String acTemp = loadState("ac_temp");
    if (acTemp != "") webSocket.broadcastTXT(acTemp);

    Serial.println("Loaded states from NVS:");
    Serial.println(acTemp);
    Serial.println(acState);
    Serial.println(acPower);
    Serial.println(acMode);
}

#endif