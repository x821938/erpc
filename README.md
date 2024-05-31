# ERPC Class

The publish and subscribe methods in the ERPC class provide the functionality for sending and receiving data over a serial port using the concept of topics.

When you subscribe to a topic, you are indicating that you are interested in receiving data related to that topic. The subscribe method takes two parameters: the `topicId` and a `callback` function. The `topicId` is an identifier for the topic you want to subscribe to, and the callback function is the function that will be called when data is received on that topic. Inside the callback function, you can process the received data based on your application's logic.

Overall, the publish and subscribe methods allow you to establish communication between devices by sending and receiving data on specific topics. This enables you to implement remote procedure calls (RPC) and exchange information between devices over a serial port.

## Usage

Start looking in the example. Its easiest to start with the twoWay example on both EPS's and connect the selected pins between them. Remember to cross RX/TX.

## Public Methods

### ERPC(HardwareSerial& serialPort, uint8_t maxTopics)

This is the constructor for the `ERPC` class. It initializes the ERPC library, setting up the serial port for communication and allocating memory for the maximum number of topics that can be subscribed to. Each topic uses 3 bytes of memory.

#### Parameters

- `serialPort`: The serial port to be used for communication.
- `maxTopics`: The maximum number of topics that can be subscribed to. Default is 10

#### Example

```cpp
HardwareSerial serialPort;
uint8_t maxTopics = 62; // The absolute maximum
ERPC erpc(serialPort, maxTopics);
```

### loop()

This method processes incoming data from the serial port. It should be called in the main loop of your program to continuously process incoming data. It reads available bytes from the serial port and handles them. When data on a subscribed topic is received the registred callback will be called (Making it RCP)

#### Example

```cpp
erpc.loop();
```

### bool ERPC::subscribe(uint8*t topicId, rpcError (\_callback)(uint8_t, void*, uint8_t, rpcError))

This method is used to subscribe to a topic.

#### Parameters

- `topicId`: The ID of the topic to subscribe to. The topicId should be unique and in the range 0-62.
- `rpcError (*callback)(uint8_t, void*, uint8_t, rpcError)` Callback function data will be called when data is received on the topic.

#### Returns

- true if subscribed. False if topic ID could not be subscribed because of maxTopics is reached or Topic ID is invalid

#### Example

```cpp
uint8_t topicId = 1;
erpc.subscribe(topicId, topicHandlingCb); // Subscribe to topicId 1
```

A callback function could look like this:

```cpp
rpcError topicHandlingCb(uint8_t topicId, void* data, uint8_t length, rpcError errorCode)
{
     if (errorCode == NO_ERROR) {
        memcpy(&someVariable, data, length); // data will disappear when returning from this cb
    } else {
        Serial.printf("Error: %d", errorCode);
    }
    return errorCode; // You could return PROCESSING_ERROR if the data is not as expected
}
```

### unsubscribe(uint8_t topicId)

This method is used to unsubscribe from a topic. The topic ID should be unique and in the range 0-62.

#### Parameters

- `topicId`: The ID of the topic to unsubscribe from.

#### Returns

- true if unsubscribed. False if topic ID is not currently subscribed to or is invalid.

#### Example

```cpp
uint8_t topicId = 1;
bool success = erpc.unsubscribe(topicId);
if (success) {
    // Successfully unsubscribed
} else {
    // Failed to unsubscribe
}
```

### rpcError publish(uint8_t topicId, void\* data, uint8_t length, bool reqiureAcknowledge = false, uint16_t timeoutMs = 200);

This method is used to publish a message to a topic. The topic ID should be unique and in the range 0-62. The message should be a null-terminated string.

#### Parameters

- `topicId`: The ID of the topic to publish to.
- `data`: A pointer to the data to send. You should cast this to the correct variable/struct/whatever at the receiving end.
- `requireAcknowledge`: If true, we will wait for the receiver to confirm reception. If false we will assume the receiver got the data.
- `timeoutMs`: The maximum time to wait for acknowledge from receiver.

#### Returns

- status of the transmission. possible values: NO_ERROR, NOT_SUBSCRIBED_ERROR, CRC_ERROR, FRAME_TYPE_ERROR, ACK_TIMEOUT_ERROR, PROCESSING_ERROR

#### Example

```cpp
uint8_t topicId = 1;
const char* message = "Hello, world!";
rpcError status = erpc.publish(topicId, message, sizeof(message), true);
if (status == NO_ERROR) {
    // Successfully published
} else {
    // Failed to publish
}
```
