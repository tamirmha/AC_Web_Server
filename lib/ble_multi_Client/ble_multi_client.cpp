#include "ble_multi_client.h"


// Define static members
bool BLEClientMulti::notified = false;
int BLEClientMulti::notifty_index = -1;
std::string BLEClientMulti::per_voltage;

class ScanCallbacks : public NimBLEScanCallbacks {
public:
    explicit ScanCallbacks(BLEClientMulti* parent) : _parent(parent) {}

    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
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
    explicit ClientCallbacks(BLEClientMulti* parent) : _parent(parent){}
    // void onConnect(NimBLEClient* pClient) override {
    //     Serial.println("Connected to the server.");
    // }
    void onDisconnect(NimBLEClient* pClient, int reason) override {
        // Serial.println("Disconnected from the server.");
        _parent->onPeripheralDisconnected(pClient);
    }
private:
    BLEClientMulti* _parent;
};

void BLEClientMulti::addTargetDevice(const std::string& identifier) {
    targetDevices.push_back(identifier);
}

BLEClientMulti::BLEClientMulti()= default;

void BLEClientMulti::init() {
    Serial.println("Initializing BLE Client...");
    // add target devices - using MAC addresses
    addTargetDevice(AC_MAC);        
    addTargetDevice(PARENTS_ROOM_DUMPER_MAC);   
    addTargetDevice(WORKING_ROOM_DUMPER_MAC);   
    addTargetDevice(SAFE_ROOM_DUMPER_MAC);   
    addTargetDevice(TEST1_MAC);     
    addTargetDevice(TEST2_MAC);     
    NimBLEDevice::init("BLE MASTER - AC Remote Server");
    pBLEScan = NimBLEDevice::getScan();
    pBLEScan->setScanCallbacks(new ScanCallbacks(this)); // Corrected function call
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99);
    Serial.println("BLE Client initialized!");
}

void BLEClientMulti::startScanning() const {
    pBLEScan->start(3000, true, true);  
}

NimBLEClient* BLEClientMulti::getClientForDamper(int damperIndex) const {
    auto it = damperMacMap.find(damperIndex);
    if (it != damperMacMap.end())   return getClientForIdentifier(it->second);
    return nullptr;
}

void BLEClientMulti::connectToDevice() {
    if (isTargetDevice(MyAdvertisedDevice)) {
        // Serial.println("BLE Target device found.");
        NimBLEClient* pClient;
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks(this)); // Set client callbacks
        if (pClient->connect(MyAdvertisedDevice, true)) {
            NimBLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);

            if (pRemoteService) {
                // Get the server MAC address
                std::string serverMac = MyAdvertisedDevice->getAddress().toString();
                notify_characteristic(pRemoteService, VOLTAGE_UUID, serverMac);
                // Store the client in the map
                std::string server_name = getIndexFromMac(serverMac);
                // if (server_name=="AC") AC_CONNECTED = true; 
                // else if (server_name=="PARENTS_ROOM_DUMPER") PARENTS_ROOM_DUMPER_CONNECTED = true;
                // else if (server_name=="WORKING_ROOM_DUMPER") WORKING_ROOM_DUMPER_CONNECTED = true;
                // else if (server_name=="SAFE_ROOM_DUMPER") SAFE_ROOM_DUMPER_CONNECTED = true;
                Serial.printf("Adding client for device: %s\n", server_name.c_str());
                clients[serverMac] = pClient;

                int i = -1;          
                if (server_name=="AC") i = 0; 
                else if (server_name=="PARENTS_ROOM_DUMPER") i = 1;
                else if (server_name=="WORKING_ROOM_DUMPER") i = 2;
                else if (server_name=="SAFE_ROOM_DUMPER") i = 3;;
                if (pendingCommands[i].pending) 
                {
                    NimBLERemoteCharacteristic* pChar = pRemoteService->getCharacteristic(pendingCommands[i].characteristicUUID);
                    if (pChar) {
                        pChar->writeValue(pendingCommands[i].value);
                        Serial.println("Executed pending command");
                        pendingCommands[i].pending = false;
                    }
                }
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
    // Serial.println("Handling peripheral disconnection...");
    pClient->disconnect();
    NimBLEDevice::deleteClient(pClient);
    // Remove the client from the map
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        if (it->second == pClient) {
            // Serial.printf("Removing client for device: %s\n", it->first.c_str());
            // if (it->first == "AC") AC_CONNECTED  = false;
            // else if (it->first == "PARENTS_ROOM_DUMPER") PARENTS_ROOM_DUMPER_CONNECTED = false;
            // else if (it->first == "WORKING_ROOM_DUMPER") WORKING_ROOM_DUMPER_CONNECTED = false;
            // else if (it->first == "SAFE_ROOM_DUMPER") SAFE_ROOM_DUMPER_CONNECTED = false;
            clients.erase(it);
            break;
        }
    }
}

bool BLEClientMulti::isConnected() const {
    for (const auto& pair : clients)
    {
        const auto& client = pair.second; // Access the value (client) explicitly
        if (client && client->isConnected())
            return true;
    }   
    return false;
}

void BLEClientMulti::notify_characteristic(NimBLERemoteService* pService, const char* characteristic_uuid, std::string& serverMac)
{
    // Enable notifications for the characteristic
    NimBLERemoteCharacteristic* pCharacteristic = pService->getCharacteristic(characteristic_uuid);
    if (pCharacteristic) {
        pCharacteristic->subscribe(true, [serverMac](NimBLERemoteCharacteristic* pCharacteristic, const uint8_t* pData, size_t length, bool isNotify)
        {
            std::string voltage(reinterpret_cast<const char*>(pData), length);
            notified = true;
            per_voltage = voltage;
            Serial.println(serverMac.c_str());
            // Switch case for different MAC addresses
            if (serverMac == AC_MAC)                        notifty_index = 0;
            else if (serverMac == PARENTS_ROOM_DUMPER_MAC)  notifty_index = 1;
            else if (serverMac == WORKING_ROOM_DUMPER_MAC)  notifty_index = 2;
            else if (serverMac == SAFE_ROOM_DUMPER_MAC)     notifty_index = 3;
            else if (serverMac == TEST1_MAC)                notifty_index = 4;
            else if (serverMac == TEST2_MAC)                notifty_index = 5;
            else  Serial.println("Notified from unknown device!");

            Serial.print("Notified Data: ");
            Serial.println(voltage.c_str());
            Serial.print("Server MAC: ");
            Serial.println(serverMac.c_str());
        });
    }
    else  Serial.println("Characteristic NOT found!");
}

NimBLEClient* BLEClientMulti::getClientForIdentifier(const std::string& identifier) const {
    auto it = clients.find(identifier);
    return (it != clients.end()) ? it->second : nullptr;
}