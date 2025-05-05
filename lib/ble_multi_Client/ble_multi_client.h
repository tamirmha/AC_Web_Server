#ifndef BLE_MULTI_CLIENT_H
#define BLE_MULTI_CLIENT_H

#define CONFIG_BT_NIMBLE_MAX_CONNECTIONS 4 

#include <NimBLEDevice.h>
#include <NimBLEScan.h>
#include <NimBLEAdvertisedDevice.h>
#include <NimBLEClient.h>
#include <vector>
#include <map>
#include "Arduino.h"
#include <unordered_map> // Include for dynamic client storage

// UUIDs for BLE services and characteristics
#define SERVICE_UUID "5678abcd-0000-1000-8000-00805f9b34fb"
#define VENT_SPEED_UUID "5678abcd-0001-1000-8000-00805f9b34fb"
#define MODE_UUID "5678abcd-0002-1000-8000-00805f9b34fb"
#define STATE_UUID "5678abcd-0003-1000-8000-00805f9b34fb"
#define TEMP_UUID "5678abcd-0004-1000-8000-00805f9b34fb"
#define VOLTAGE_UUID "5678abcd-0005-1000-8000-00805f9b34fb"

// BLE MAC addresses for specific devices
static const char* AC_MAC = "9c:9e:6e:c1:0c:5e"; // Air Conditioner
static const char* DUMPER1_MAC = "9c:9e:6e:c1:09:e2"; // Parents room damper
static const char* DUMPER2_MAC = "64:e8:33:8c:04:a6"; // Working room damper
static const char* DUMPER3_MAC = "64:e8:33:8a:7c:be"; // Safe room damper
static const char* TEST1_MAC = "DC:06:75:E9:6F:92"; // Redundant Controller
static const char* TEST2_MAC = "DC:06:75:E9:3C:02"; // Tests Controller


// Map of damper indices to their corresponding MAC addresses
static const std::unordered_map<int, std::string> damperMacMap = {
    {0, AC_MAC},
    {1, DUMPER1_MAC},
    {2, DUMPER2_MAC},
    {3, DUMPER3_MAC},
    {4, TEST1_MAC},
    {5, TEST2_MAC}
};

/**
 * @class BLEClientMulti
 * @brief A class to manage multiple BLE client connections and interactions.
 */
class BLEClientMulti {
public:
    NimBLEScan* pBLEScan{}; ///< Pointer to the BLE scanner instance.
    std::unordered_map<std::string, NimBLEClient*> clients; ///< Map to store BLE clients by MAC address or identifier.
    NimBLEAdvertisedDevice* MyAdvertisedDevice{}; ///< Pointer to the currently advertised BLE device.
    std::vector<std::string> targetDevices; ///< List of target devices to connect to.
    bool doConnect = false; ///< Flag indicating whether to initiate a connection.
    static bool notified; ///< Static flag indicating if a notification was received.
    static std::string per_voltage; ///< Static variable to store the voltage value from the peripheral.
    static int notifty_index; ///< Static variable to store the index of the notifying device.

    /**
     * @brief Constructor for BLEClientMulti.
     */
    BLEClientMulti();

    /**
     * @brief Initialize the BLE client and scanner.
     */
    void init();

    /**
     * @brief Add a target device to the list of devices to connect to.
     * @param identifier The identifier (e.g., MAC address) of the target device.
     */
    void addTargetDevice(const std::string& identifier);

    /**
     * @brief Start scanning for BLE devices.
     */
    void startScanning() const;

    /**
     * @brief Connect to a target BLE device.
     */
    void connectToDevice();

    /**
     * @brief Check if the advertised device is a target device.
     * @param advertisedDevice Pointer to the advertised BLE device.
     * @return True if the device is a target device, false otherwise.
     */
    bool isTargetDevice(NimBLEAdvertisedDevice* advertisedDevice);

    /**
     * @brief Handle the event when a peripheral device is disconnected.
     * @param pClient Pointer to the BLE client that was disconnected.
     */
    void onPeripheralDisconnected(NimBLEClient* pClient);

    /**
     * @brief Check if any BLE client is currently connected.
     * @return True if a client is connected, false otherwise.
     */
    bool isConnected() const;

    /**
     * @brief Notify a characteristic of a BLE service.
     * @param pService Pointer to the remote BLE service.
     * @param characteristic_uuid UUID of the characteristic to notify.
     * @param serverMac MAC address of the server device.
     */
    void notify_characteristic(NimBLERemoteService* pService, const char* characteristic_uuid, std::string& serverMac);

    /**
     * @brief Get the BLE client for a specific damper by index.
     * @param damperIndex Index of the damper.
     * @return Pointer to the BLE client for the damper.
     */
    NimBLEClient* getClientForDamper(int damperIndex) const;

    /**
     * @brief Get the BLE client for a specific identifier (e.g., MAC address).
     * @param identifier The identifier of the target device.
     * @return Pointer to the BLE client for the identifier.
     */
    NimBLEClient* getClientForIdentifier(const std::string& identifier) const;
};

#endif
