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
    erpc.publish(0, &localCounter, sizeof(localCounter));
    erpc.loop();

    localCounter++;
}

rpcError rxCallback(uint8_t topicId, void* data, uint8_t length, rpcError errorCode)
{
    static uint32_t lastPrint = 0;

    if (errorCode == NO_ERROR) {
        if (length == sizeof(remoteCounter)) {
            memcpy(&remoteCounter, data, length);
            rxSuccess++;
        } else {
            rxErrors++;
            Serial.printf("Received data with unexpected length: %d\n", length);
        }
    } else {
        rxErrors++;
        Serial.printf("Received data with error code: %d, length: %d, topicId: %d, data: %d\n", errorCode, length, topicId, *(uint32_t*)data);
    }

    if (millis() - lastPrint > 1000) {
        Serial.printf("Received... Success count: %d, Error count: %d, Local Counter: %d, Remote Counter: %d\n", rxSuccess, rxErrors, localCounter, remoteCounter);
        lastPrint = millis();
    }
    return errorCode; // You could return PROCESSING_ERROR if the data is not as expected
}