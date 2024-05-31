#include <Arduino.h>
#include <ERPC.h>
#include <HardwareSerial.h>

#define SERIAL_RX_PIN 21
#define SERIAL_TX_PIN 22
#define SERIAL_BAUD_RATE 115200

void makeRandomString(char* testSendChar);

HardwareSerial rpcTxPort(1);
ERPC erpc(rpcTxPort);

struct TestStruct {
    uint16_t random;
    uint16_t counter;
};

void setup()
{
    Serial.begin(115200);

    rpcTxPort.begin(SERIAL_BAUD_RATE, SERIAL_8N1, SERIAL_RX_PIN, SERIAL_TX_PIN);
}

void loop()
{
    static uint32_t txCount = 0;
    static TestStruct testSendStruct;
    static char testSendChar[100];
    static rpcError err;

    testSendStruct.random = random(0, 0xffff);
    testSendStruct.counter = txCount;

    Serial.printf("\nSending TestStruct: random = %d, counter = %d\n", testSendStruct.random, testSendStruct.counter);
    err = erpc.publish(0, &testSendStruct, sizeof(testSendStruct), true);
    if (err != NO_ERROR) {
        Serial.printf("Could not send TestStruct. Error: %d\n", err);
    } else {
        Serial.printf("Sent successful. Acknowledge received\n");
    }

    makeRandomString(testSendChar);
    Serial.printf("\nSending random string: %s\n", testSendChar);
    err = erpc.publish(1, testSendChar, strlen(testSendChar) + 1, true); // +1 for null terminator
    if (err != NO_ERROR) {
        Serial.printf("Could not send random string. Error: %d\n", err);
    } else {
        Serial.printf("Sent successful. Acknowledge received\n");
    }

    erpc.loop();
    delay(3000);

    txCount++;
}

void makeRandomString(char* testSendChar)
{
    int randomStringLength = random(1, 101); // Generate random length between 1 and 100
    for (int i = 0; i < randomStringLength; i++) {
        testSendChar[i] = random(97, 123); // Generate random ASCII character between 97 ('a') and 122 ('z')
    }
    testSendChar[randomStringLength] = '\0'; // Add null terminator at the end of the string
}
