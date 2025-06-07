#ifndef MYBLYNK_H
#define MYBLYNK_H

#include "global_var.h"

// Virtual Pins
#define VPIN_POWER          V0
#define VPIN_DAMPER1        V1
#define VPIN_DAMPER2        V2
#define VPIN_DAMPER3        V3
#define VPIN_TEMPERATURE    V4
#define VPIN_POWER_STATE    V5
#define VPIN_AC_STATE       V6
#define AC_VOLTAGE          V7
#define DAMPER1_VOLTAGE     V8
#define DAMPER2_VOLTAGE     V9
#define DAMPER3_VOLTAGE     V10

// =========== VIRTUAL WRITE HANDLERS =============

// AC Power (V0)
BLYNK_WRITE(VPIN_POWER) {
    sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_POWER), param.asInt() ? "on" : "off");
}


// Damper 1 Power (V1)
BLYNK_WRITE(VPIN_DAMPER1) {
    sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_DAMPER1), param.asInt() ? "on" : "off");
}

// Damper 2 Power (V2)
BLYNK_WRITE(VPIN_DAMPER2) {
        sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_DAMPER2), param.asInt() ? "on" : "off");
}

// Damper 3 Power (V3)
BLYNK_WRITE(VPIN_DAMPER3) {
        sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_DAMPER3),
                             param.asInt() ? "on" : "off");
}

// Temperature slider (V4)
BLYNK_WRITE(VPIN_TEMPERATURE) {
    sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_POWER), param.asInt() ? "on" : "off");
}

// AC Mode (V5)
BLYNK_WRITE(VPIN_POWER_STATE) {
    sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_POWER), param.asInt() ? "on" : "off");
}

// Fan Speed or Mode (V6)
BLYNK_WRITE(VPIN_AC_STATE) {
    sendDataToPeripheral(NimBLEUUID(STATE_UUID), bleClient.getClientForDamper(VPIN_POWER), param.asInt() ? "on" : "off");
}


#endif //MYBLYNK_H
