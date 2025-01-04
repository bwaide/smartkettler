#ifndef BLECOMMUNICATION_H
#define BLECOMMUNICATION_H

#include <NimBLEDevice.h>

// Function prototypes
void setupBLE(const char* deviceName, const char* serialNumber, const char* firmwareVersion);

void pushData(
    uint16_t rotationMarkersCrank, 
    uint32_t rotationMarkersSimWheel, 
    uint16_t lastTransitionTimeSimWheel, 
    uint16_t lastTransitionTimeCrank
);

#endif // BLECOMMUNICATION_H
