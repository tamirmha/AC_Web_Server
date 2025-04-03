#include "ble_multi_client.h"


// Define static members
bool BLEClientMulti::notified = false;
int BLEClientMulti::notifty_index = -1;
std::string BLEClientMulti::per_voltage;

class ScanCallbacks : public NimBLEScanCallbacks {
public:
    explicit ScanCallbacks(BLEClientMulti* parent) : _parent(parent) {}

    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
//        Serial.print("Advertised Device found: ");
//        Serial.println(advertisedDevice->getAddress().toString().c_str());
        _parent->doConnect = true;
        _parent->MyAdvertisedDevice = const_cast<NimBLEAdvertisedDevice*>(advertisedDevice);
    }
    void onScanEnd(const NimBLEScanResults& results, int reason) override {  // Matching signature
        Serial.printf("Scan ended (Reason: %d), restarting...\n", reason);
        _parent->startScanning();  // Restart scan indefinitely
    }

private:
    BLEClientMulti* _parent;
};

class ClientCallbacks : public NimBLEClientCallbacks {
public:
//    ClientCallbacks(BLEClientMulti* parent) : _parent(parent) {}
    ClientCallbacks()= default;

    void onConnect(NimBLEClient* pClient) override {
        Serial.println("Connected to the server.");
    }

    void onDisconnect(NimBLEClient* pClient, int reason) override {
        Serial.println("Disconnected from the server.");
        BLEClientMulti::onPeripheralDisconnected(pClient);
    }

//private:
//    BLEClientMulti* _parent;
};

void BLEClientMulti::addTargetDevice(const std::string& identifier) {
    targetDevices.push_back(identifier);
}

BLEClientMulti::BLEClientMulti()= default;

void BLEClientMulti::init() {
    Serial.println("Initializing BLE Client...");
    // add target devices - using MAC addresses
    addTargetDevice(AC_MAC);        // Add target BLE device by name or MAC
    addTargetDevice(DUMPER1_MAC);   // Add target BLE device by name or MAC
    addTargetDevice(DUMPER2_MAC);   // Add target BLE device by name or MAC
    addTargetDevice(DUMPER3_MAC);   // Add target BLE device by name or MAC
    NimBLEDevice::init("BLE MASTER - WebSocket Server");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(new ScanCallbacks(this)); // Corrected function call
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    Serial.println("BLE Client initialized!");
}

void BLEClientMulti::startScanning() const {
    pBLEScan->start(3000, false, true);  // Scan indefinitely
}

NimBLEClient* BLEClientMulti::getClientForDamper(int damperIndex) const {
    switch (damperIndex) {
        case 1:
            return d1Client;
        case 2:
            return d2Client;
        case 3:
            return d3Client;
        default:
            return nullptr;
    }
}
void BLEClientMulti::connectToDevice() {
    if (isTargetDevice(MyAdvertisedDevice)) {
        Serial.println("BLE Target device found.");
        NimBLEClient* pClient;
        pClient = NimBLEDevice::createClient();
//        pClient->setClientCallbacks(new ClientCallbacks(this)); // Set client callbacks
        pClient->setClientCallbacks(new ClientCallbacks()); // Set client callbacks
        if (pClient->connect(MyAdvertisedDevice)) {
            Serial.println("Connected to BLE peripheral.");
            NimBLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);

            if (pRemoteService) {
                Serial.println("Service UUID set.");
                // Get the server MAC address
                std::string serverMac = MyAdvertisedDevice->getAddress().toString();
                notify_characteristic(pRemoteService, VOLTAGE_UUID, serverMac);

                // Switch case for different MAC addresses
                if (serverMac == AC_MAC)            acClient = pClient;
                else if (serverMac == DUMPER1_MAC)  d1Client = pClient;
                else if (serverMac == DUMPER2_MAC)  d2Client = pClient;
                else if (serverMac == DUMPER3_MAC)  d3Client = pClient;
                else  Serial.println("Connected to an unknown device.");
            }
        } else {
            Serial.println("Unable to connect to BLE peripheral.");
            NimBLEDevice::deleteClient(pClient); // Corrected deletion
        }
    }
}

bool BLEClientMulti::isTargetDevice(NimBLEAdvertisedDevice* advertisedDevice) {
    std::string address = advertisedDevice->getAddress().toString();
    std::string name = advertisedDevice->getName();
    for (const auto& target : targetDevices)
        if (target == address || target == name)    return true;
    return false;
}

// New method to handle peripheral disconnection
void BLEClientMulti::onPeripheralDisconnected(NimBLEClient* pClient) {
    Serial.println("Handling peripheral disconnection...");
    NimBLEDevice::deleteClient(pClient);
}
