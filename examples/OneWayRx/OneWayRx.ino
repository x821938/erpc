/* Receiver example for the eRPC library. This example demonstrates how to receive data from the sender.
 *
 * The sender sends a struct and a string to the receiver. The receiver has a callback function that is called when data is received.
 * The callback function checks the topic ID and length of the received data and prints the data if it is as expected.
 * The sender sends the struct and string every 2 seconds. We should see the received data printed on the serial monitor.
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
    erpc.subscribe(0, rxCallback); // Topic ID 0 for test struct. The sender sends the struct with topic ID 0
    erpc.subscribe(1, rxCallback); // Topic ID 1 for test string. The sender sends the string with topic ID 1
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