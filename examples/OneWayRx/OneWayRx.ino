#include <Arduino.h>
#include <ERPC.h>
#include <HardwareSerial.h>

#define SERIAL_RX_PIN 21
#define SERIAL_TX_PIN 22
#define SERIAL_BAUD_RATE 115200

HardwareSerial rpcTxPort(1);
ERPC erpc(rpcTxPort);
rpcError rxCallback(uint8_t topicId, void* data, uint8_t length, rpcError errorCode); // declare the callback function

struct TestStruct {
    uint16_t random;
    uint16_t counter;
};

TestStruct testReceiveStruct;
char testReceiveChar[100];

void setup()
{
    Serial.begin(115200);

    rpcTxPort.begin(SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);
    erpc.subscribe(0, rxCallback); // Topic ID 0 for test struct
    erpc.subscribe(1, rxCallback); // Topic ID 1 for test string
}

void loop()
{
    erpc.loop();
}

rpcError rxCallback(uint8_t topicId, void* data, uint8_t length, rpcError errorCode)
{
    if (errorCode == NO_ERROR) {
        if (topicId == 0 && length <= sizeof(testReceiveStruct)) { // id 0 is for struct
            memcpy(&testReceiveStruct, data, sizeof(testReceiveStruct));
            Serial.printf("Received struct. Random: %d, Counter: %d\n", testReceiveStruct.random, testReceiveStruct.counter);
        } else if (topicId == 1 && length <= sizeof(testReceiveChar)) { // id 1 is for string
            memcpy(testReceiveChar, data, length);
            Serial.printf("Received string: %s\n\n", testReceiveChar);
        }
    } else {
        Serial.printf("Received Error: %d\n", errorCode);
    }
    return errorCode; // You could return PROCESSING_ERROR if the data is not as expected
}