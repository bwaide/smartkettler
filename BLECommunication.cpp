#include "BLECommunication.h"

#include <NimBLEDevice.h>

#define CONFIG_BT_NIMBLE_DEBUG_LOGS 1

struct CSCMeasurement_t
{
    uint8_t flags; // bit 0: wheel data present; bit 1: crank data present
    uint32_t wheelRevolutions;
    uint16_t lastWheelEvent;
    uint16_t crankRevolutions;
    uint16_t lastCrankEvent;
} __packed;

CSCMeasurement_t cscMeasurement = {.flags = (1 << 1) | (1 << 0)}; // wheel and crank data

NimBLECharacteristic cscMeasurementCharacteristics(NimBLEUUID((uint16_t)0x2A5B), NIMBLE_PROPERTY::NOTIFY);
NimBLECharacteristic *serialNumberCharacteristic;
NimBLECharacteristic *firmwareVersionCharacteristic;

NimBLEServer *pServer; // Declare globally

bool bleConnected = false;

portMUX_TYPE bleMutex = portMUX_INITIALIZER_UNLOCKED;

void pushData(
    uint16_t rotationMarkersCrank, 
    uint32_t rotationMarkersSimWheel, 
    uint16_t lastTransitionTimeSimWheel, 
    uint16_t lastTransitionTimeCrank
)
{

    if (!bleConnected)
    {
        return;
    }

    noInterrupts();
    uint16_t scaledCrankRevolutions = rotationMarkersCrank;
    uint32_t scaledWheelRevolutions = rotationMarkersSimWheel;
    uint16_t scaledLastWheelEvent = (uint16_t)((lastTransitionTimeSimWheel & 0xFFFF) * 1024 / 1000);
    uint16_t scaledLastCrankEvent = (uint16_t)((lastTransitionTimeCrank & 0xFFFF) * 1024 / 1000);
    interrupts();

    portENTER_CRITICAL(&bleMutex);
    cscMeasurement.crankRevolutions = scaledCrankRevolutions;
    cscMeasurement.wheelRevolutions = scaledWheelRevolutions;
    cscMeasurement.lastWheelEvent = scaledLastWheelEvent;
    cscMeasurement.lastCrankEvent = scaledLastCrankEvent;

    try
    {
        cscMeasurementCharacteristics.setValue((uint8_t *)&cscMeasurement, sizeof(cscMeasurement));
        if (cscMeasurementCharacteristics.notify())
        {
            Serial.print(".");
        }
        else
        {
            Serial.println("BLE Notification failed.");
        }
    }
    catch (std::exception &e)
    {
        Serial.println("BLE Notification failed.");
        Serial.println(e.what());
    }
    portEXIT_CRITICAL(&bleMutex);
}

class MyServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override
    {
        Serial.println("Connected");
        bleConnected = true;
        pServer->getAdvertising()->stop(); // Stop advertising when connected
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo, int reason) override
    {
        Serial.println("Disconnected");
        bleConnected = false;
        delay(100); // Add a short delay before restarting advertising
        pServer->getAdvertising()->start();
    }
};

void setupBLE(const char* deviceName, const char* serialNumber, const char* firmwareVersion)
{
    NimBLEDevice::init(deviceName);
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    NimBLEService *pCadence = pServer->createService(NimBLEUUID((uint16_t)0x1816));
    pCadence->addCharacteristic(&cscMeasurementCharacteristics);
    pCadence->start();

    // Add Device Information Service
    NimBLEService *deviceInfoService = pServer->createService(NimBLEUUID((uint16_t)0x180A));
    serialNumberCharacteristic = deviceInfoService->createCharacteristic(
        NimBLEUUID((uint16_t)0x2A25), // Serial Number String
        NIMBLE_PROPERTY::READ);
    firmwareVersionCharacteristic = deviceInfoService->createCharacteristic(
        NimBLEUUID((uint16_t)0x2A26), // Firmware Revision String
        NIMBLE_PROPERTY::READ);
    serialNumberCharacteristic->setValue(serialNumber);
    firmwareVersionCharacteristic->setValue(firmwareVersion);

    NimBLEDescriptor *firmwareDescriptor = new NimBLEDescriptor((uint16_t)0x2901, NIMBLE_PROPERTY::READ, 20, firmwareVersionCharacteristic);
    firmwareDescriptor->setValue("Firmware Revision");
    firmwareVersionCharacteristic->addDescriptor(firmwareDescriptor);

    NimBLEDescriptor *serialDescriptor = new NimBLEDescriptor((uint16_t)0x2901, NIMBLE_PROPERTY::READ, 20, serialNumberCharacteristic);
    serialDescriptor->setValue("Serial Number");
    serialNumberCharacteristic->addDescriptor(serialDescriptor);

    deviceInfoService->start();

    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pCadence->getUUID());
    pAdvertising->addServiceUUID(deviceInfoService->getUUID());
    pAdvertising->addServiceUUID(cscMeasurementCharacteristics.getUUID());

    NimBLEAdvertisementData scanResponseData;
    scanResponseData.setName(deviceName);
    pAdvertising->setScanResponseData(scanResponseData);

    NimBLEDevice::startAdvertising();
}