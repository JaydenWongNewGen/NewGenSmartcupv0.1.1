#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class BluetoothManager : public BLEServerCallbacks {
public:
    void begin();
    bool isDeviceConnected() const;
    void notifyJSON(const String& json);

private:
    BLEServer* pServer = nullptr;
    BLEService* pService = nullptr;
    BLECharacteristic* pCharacteristic = nullptr;
    BLE2902* pCCCD = nullptr;

    bool deviceConnected = false;

    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
};

#endif
