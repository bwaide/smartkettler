/*
 * MIT License
 * 
 * Stripped down bike computer implementation transmitting speed and cadence via BLE
 */

#include "BLECommunication.h"

#define DEVICE_NAME "SmartKettler"
#define SERIAL_NUMBER "250101-BWA"
#define FIRMWARE_VERSION "1.0.0"

const uint32_t SENSOR_PIN = 13; // Pin connected to the Hall sensor

const uint32_t UPDATE_INTERVAL = 1000; // Update interval in milliseconds

const float SMALL_WHEEL_CIRCUMFERENCE = 0.591; // Measured circumference of the spinning wheel in meters
const float STANDARD_WHEEL_CIRCUMFERENCE = 2.07; // Standard bike wheel circumference in meters
const float WHEEL_SCALING_FACTOR = SMALL_WHEEL_CIRCUMFERENCE / STANDARD_WHEEL_CIRCUMFERENCE; // Scales measured wheel revolutions and timing to simulate a standard bike wheel
const float CRANK_SCALING_FACTOR = 1 / 9.8;

volatile unsigned long lastTransitionTimeWheel = 0; // Time of the last positive edge
volatile unsigned long lastTransitionTimeSimWheel = 0; // Time of the last positive edge
volatile unsigned long lastTransitionTimeCrank = 0; // Time of the last positive edge for the simulated crank 

uint32_t rotationMarkersWheel = 0;
uint32_t rotationMarkersSimWheel = 0;
uint32_t rotationMarkersCrank = 0;

portMUX_TYPE isrMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onSignalChange() {
    
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastTransitionTimeWheel;

    if (deltaTime <= 0) {
      return;
    }

    noInterrupts();
    portENTER_CRITICAL_ISR(&isrMux);

    rotationMarkersWheel++;
    lastTransitionTimeWheel = currentTime;

    // Simulate crank: Increment crank revolutions for every 10 wheel revolutions
    static float accumulatedCrankRotations = 0; // Tracks partial rotations
    
    accumulatedCrankRotations += CRANK_SCALING_FACTOR;
    // Add the simulated rotations
    if (accumulatedCrankRotations >= 1.0) {
        int fullRotations = (int)accumulatedCrankRotations;
        rotationMarkersCrank++;
        
        // Calculate the exact fraction of deltaTime contributing to the last simulated rotation
        float partialRotation = accumulatedCrankRotations - fullRotations;
        lastTransitionTimeCrank = currentTime - (unsigned long)(partialRotation * deltaTime);

        // Update the accumulated rotations
        accumulatedCrankRotations -= fullRotations;
    }

    // Simulate normal bike wheel
    static float accumulatedSimWheelRotations = 0; // Tracks partial rotations
    
    accumulatedSimWheelRotations += WHEEL_SCALING_FACTOR;
    // Add the simulated rotations
    if (accumulatedSimWheelRotations >= 1.0) {
        int fullRotations = (int)accumulatedSimWheelRotations;
        rotationMarkersSimWheel++;
        
        // Calculate the exact fraction of deltaTime contributing to the last simulated rotation
        float partialRotation = accumulatedSimWheelRotations - fullRotations;
        lastTransitionTimeSimWheel = currentTime - (unsigned long)(partialRotation * deltaTime);

        // Update the accumulated rotations
        accumulatedSimWheelRotations -= fullRotations;
    }

    portEXIT_CRITICAL_ISR(&isrMux);
    interrupts();
}

void setup() {
    Serial.begin(115200);

    setupBLE(DEVICE_NAME, SERIAL_NUMBER, FIRMWARE_VERSION);

    pinMode(SENSOR_PIN, INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(SENSOR_PIN), onSignalChange, RISING);
}

void loop() {

    pushData(rotationMarkersCrank, rotationMarkersSimWheel, lastTransitionTimeSimWheel, lastTransitionTimeCrank);  

    delay(UPDATE_INTERVAL);
}