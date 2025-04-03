#ifndef BLE_MULTI_CLIENT_H
#define BLE_MULTI_CLIENT_H

#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEClient.h>
#include <vector>
#include <map>
#include "Arduino.h"

#define SERVICE_UUID "5678abcd-0000-1000-8000-00805f9b34fb"
#define VENT_SPEED_UUID "5678abcd-0001-1000-8000-00805f9b34fb"
#define MODE_UUID "5678abcd-0002-1000-8000-00805f9b34fb"
#define STATE_UUID "5678abcd-0003-1000-8000-00805f9b34fb"
#define TEMP_UUID "5678abcd-0004-1000-8000-00805f9b34fb"
#define VOLTAGE_UUID "5678abcd-0005-1000-8000-00805f9b34fb"
const std::vector<std::string> CHARACTERISTIC_UUIDS = {VENT_SPEED_UUID, MODE_UUID, STATE_UUID, TEMP_UUID, VOLTAGE_UUID};

static const char* AC_MAC = "9c:9e:6e:c1:0c:5e";
static const char* DUMPER1_MAC = "9c:9e:6e:c1:09:e2";  // Parents room
static const char* DUMPER2_MAC = "64:e8:33:8c:04:a6";  // WORKING_ROOM_DUMPER
static const char* DUMPER3_MAC = "64:e8:33:8a:7c:be";  // SAFE_ROOM_DUMPER


class BLEClientMulti {

static void notify_characteristic(NimBLERemoteService* pService, const char* characteristic_uuid, std::string& serverMac)
{
    // Enable notifications for the characteristic
    NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(characteristic_uuid);
    if (pCharacteristic) {
        pCharacteristic->subscribe(true, [serverMac](NimBLERemoteCharacteristic* pCharacteristic, const uint8_t* pData, size_t length, bool isNotify)
        {
            std::string voltage(reinterpret_cast<const char*>(pData), length);
            notified = true;
            per_voltage = voltage;

            // Switch case for different MAC addresses
            if (serverMac == AC_MAC)            notifty_index = 0;
            else if (serverMac == DUMPER1_MAC)  notifty_index = 1;
            else if (serverMac == DUMPER2_MAC)  notifty_index = 2;
            else if (serverMac == DUMPER3_MAC)  notifty_index = 3;
            else  Serial.println("Notified from unknown device!");

            Serial.print("Notified Data: ");
            Serial.println(voltage.c_str());
            Serial.print("Server MAC: ");
            Serial.println(serverMac.c_str());
        });
        Serial.println("Subscribed to notifications!");
    }
    else  Serial.println("Characteristic NOT found!");
}

public:
    NimBLEScan* pBLEScan{};
    NimBLEClient* acClient{};
    NimBLEClient* d1Client{};
    NimBLEClient* d2Client{};
    NimBLEClient* d3Client{};
    NimBLEAdvertisedDevice* MyAdvertisedDevice{};
    std::vector<std::string> targetDevices;  // List of target devices to connect to

    bool doConnect = false;
    static bool notified;
    static std::string per_voltage;
    static int notifty_index;

    BLEClientMulti();
    void init();
    void addTargetDevice(const std::string& identifier);
    void startScanning() const;
    void connectToDevice();
    bool isTargetDevice(NimBLEAdvertisedDevice* advertisedDevice);
    static void onPeripheralDisconnected(NimBLEClient* pClient);
    NimBLEClient* getClientForDamper(int damperIndex) const ;
};

#endif
