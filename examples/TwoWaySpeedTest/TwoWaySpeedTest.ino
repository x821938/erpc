/* This example demonstrates how to use the eRPC library to send and receive data between two devices. The example uses two ESP32 devices that both acts as a transmitter and a receiver at the same time.
 * This is a high speed test. Both ends sends as fast as possible.
 *
 * The first device sends a counter value to the receiver, and other device sends it's counter value. The transmitter increments the counter value by one each time it sends the data.
 *
 * You have to wire the two devices together using the UART pins. The transmitter's TX pin should be connected to the receiver's RX pin, and the transmitter's RX pin should be connected to the receiver's TX pin.
 * Change the SERIAL_RX_PIN and SERIAL_TX_PIN to the correct pins for your setup. The example uses the HardwareSerial library to communicate with the other device.
 */

#include <Arduino.h>
#include <ERPC.h>
#include <HardwareSerial.h>

#define SERIAL_RX_PIN 21
#define SERIAL_TX_PIN 22
#define SERIAL_BAUD_RATE 115200

rpcError rxCallback(uint8_t topicId, void* data, uint8_t length, rpcError errorCode); // Declare the callback function

HardwareSerial rpcTxPort(1);
ERPC erpc(rpcTxPort);

uint32_t localCounter = 0;
uint32_t remoteCounter = 0;

uint32_t rxErrors = 0;
uint32_t rxSuccess = 0;

void setup()
{
    Serial.begin(115200);

    rpcTxPort.begin(SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);
    erpc.subscribe(0, rxCallback);
}

void loop()
{
    erpc.publish(0, &localCounter, sizeof(localCounter)); // No acknowledge required
    erpc.loop();

    localCounter++;
}

rpcError rxCallback(uint8_t topicId, void* data, uint8_t length, rpcError errorCode)
{
    static uint32_t lastPrint = 0;

    if (errorCode == NO_ERROR) { // it's your responsibility to check if there are any errors and the data is as expected
        if (length == sizeof(remoteCounter)) {
            memcpy(&remoteCounter, data, length); // save data data, because it will be destroyed after the callback
            rxSuccess++;
        } else {
            rxErrors++;
            Serial.printf("Received data with unexpected length: %d\n", length);
        }
    } else {
        rxErrors++;
        Serial.printf("Received data with error code: %d, length: %d, topicId: %d, data: %d\n", errorCode, length, topicId, *(uint32_t*)data);
    }

    // Every second print the success and error count
    if (millis() - lastPrint > 1000) {
        Serial.printf("Received... Success count: %d, Error count: %d, Local Counter: %d, Remote Counter: %d\n", rxSuccess, rxErrors, localCounter, remoteCounter);
        lastPrint = millis();
    }
    return errorCode; // You could return PROCESSING_ERROR if the data is not as expected
}