# SmartKettler

## Hardware used
Microcontroller: ESP32 WROOM-32D (but any ESP32 with support for BLE should work)
Hall Sensor: A3144
Resistor: 10k
Magnets: A bunch, like these: https://www.amazon.de/dp/B09W8R43Q8?ref=ppx_yo2ov_dt_b_fed_asin_title&th=1

## Libraries used
NimBLE-Arduino by h2zero (https://github.com/h2zero/NimBLE-Arduino)

## Setup
Clone repository and load SmartKettler.ino into Arduino IDE
Connect ESP32 to your computer
Choose "ESP32 Dev Module"
Set "Upload Speed" to 115200, at least I got error messages otherwise when uploading the Sketch
Compile and upload sketch
After a while in apps like "nRF Connect" for testing or "Wahoo" the device should appear as "SmartKettler"

To actually read out data you have to connect the hall sensor and the resistor to your ESP32 with the data pin connected to pin13 of your ESP32, like here: https://forum.arduino.cc/t/a3144-hall-effect-sensors-issues/1255510/7
The hall sensor has to be placed in very close proximity to the spinning wheel of your stationary bike.
The magnets have to be placed on the outer rim where they don't interfer with the rest of the mechanics of the bike. Use as many magnets you have to create a long enough signal for the sensor to detect during high speeds.
Pay attention to use the same orientation for every magnet and place them as close together as possible (they will repel if placed correctly).

## Test
For every rotation of the spinning wheel the ESP32 registers a rotation. To simulate a "real" bike the code now calculates the rotations of the crank and a "real" wheel and sends this data via BLE.
In my case the scaling factors where 9.8 for the crank (this number was even printed on the built-in computer of my Kettler bike) and 0.2855 for the wheel (figured that us by trial and error).
For your specific bike you probably have to change these numbers.

## Troubleshooting
To get BLE to work I had to be very cautious with changing and reading the variables during the interrupt and the actual sending, because at higher speeds the ESP crashed multiple times due to race conditions when reading and writing the same variable at the same time. 
To prevent that I used "noInterrupts" and Mutex at critical points. Now it works, but to be honest I'm not that confident that this is the best solution.
