#include "BluetoothManager.h"
#include <Arduino.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

void BluetoothManager::begin() {
    BLEDevice::init("SmartCup"); // match what your site expects
    // BLEDevice::setMTU(185);    // optional; skip on Windows if flaky

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(this);

    pService = pServer->createService(SERVICE_UUID);

    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |   // keep WRITE if site needs it
        BLECharacteristic::PROPERTY_NOTIFY
    );

    pCCCD = new BLE2902();
    pCCCD->setNotifications(true);
    pCharacteristic->addDescriptor(pCCCD);

    pCharacteristic->setValue("Hello from SmartCup");
    pService->start();

    BLEAdvertising* adv = pServer->getAdvertising();
    adv->addServiceUUID(SERVICE_UUID);
    adv->setScanResponse(true);
    adv->start();

    Serial.println("BLE service started.");
}

bool BluetoothManager::isDeviceConnected() const {
    return deviceConnected;
}

void BluetoothManager::onConnect(BLEServer* /*pServer*/) {
    deviceConnected = true;
    Serial.println("BLE client connected.");
}

void BluetoothManager::onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE client disconnected.");
    pServer->getAdvertising()->start();
}

// Safe notify: copy into a temp String to satisfy non-const API
void BluetoothManager::notifyJSON(const String& json) {
    if (!deviceConnected || !pCharacteristic) return;
    const size_t CHUNK = 180;
    for (size_t sent = 0; sent < json.length(); ) {
        size_t n = min(CHUNK, json.length() - sent);
        String chunk = json.substring(sent, sent + n);
        pCharacteristic->setValue((uint8_t*)chunk.c_str(), chunk.length());
        pCharacteristic->notify();
        sent += n;
        delay(1);
    }
}
