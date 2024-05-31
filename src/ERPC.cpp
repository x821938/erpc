#include <ERPC.h>

/**
 * @brief Initialize the ERPC library.
 *
 * @param serial The serial port to be used for communication.
 * @param maxTopics The maximum number of topics that can be subscribed to. keep in mind that each topic uses 3 bytes of memory. (default: 10)
 */
ERPC::ERPC(HardwareSerial& serialPort, uint8_t maxTopics)
{
    serial = &serialPort;
    maxTopics_ = maxTopics;
    topics_ = (Topic*)malloc(maxTopics_ * sizeof(Topic));
    for (size_t i = 0; i < maxTopics_; i++) {
        topics_[i].used = false;
    }
}

/**
 * @brief Process incoming data from the serial port.
 * This function should be called in the main loop to process incoming data.
 */
void ERPC::loop()
{
    while (serial->available()) {
        uint8_t byte = serialByteRead();
        stateHandle(byte);
    }
}

/**
 * @brief Subscribe to a topic.
 *
 * @param topicId The ID of the topic to subscribe to. The topic ID should be unique and in the range 0-62.
 * @param callback The callback function to be called when data is received on the topic. The callback function should have the signature: rpcError callback(uint8_t topicId, void* data, uint8_t length, rpcError errorCode).
 * You have to cast the data pointer to the appropriate type in the callback function. When you return in the callback function the data is freed. You need to make a copy of the data if you want to keep it. You can return
 * your own error code in the callback function. If can return an error code from the ERPC class.
 * @return true if the subscription was successful, false if the topic is already subscribed or there is no space in mem for the topic.
 */
bool ERPC::subscribe(uint8_t topicId, rpcError (*callback)(uint8_t, void*, uint8_t, rpcError))
{
    if (topicId > 62)
        return false; // topicId is 6 bits (0-63). 63 is reserved for ACK

    uint8_t idx = getTopicIdx(topicId);
    if (idx == 0xFF) { // topic not found, we can add it
        for (size_t i = 0; i < maxTopics_; i++) {
            if (!topics_[i].used) {
                topics_[i].topicId = topicId;
                topics_[i].callback = callback;
                topics_[i].used = true;
                return true;
            }
        } // no space in mem for the topic
    }
    return false;
}

/**
 * @brief Unsubscribe from a topic.
 *
 * @param topicId The ID of the topic to unsubscribe from.
 * @return true if the unsubscription was successful, false if the topic was not found.
 */

bool ERPC::unsubscribe(uint8_t topicId)
{
    uint8_t idx = getTopicIdx(topicId);
    if (idx != 0xFF) {
        topics_[idx].used = false;
        return true;
    }
    return false;
}

/**
 * @brief Publish data to a topic.
 *
 * @param topicId The ID of the topic to publish to.
 * @param data A pointer to the data to be published.
 * @param length The length of the data.
 * @param requireAcknowledge Whether an acknowledgement is required for the published data. Will block until an acknowledgement is received or a timeout occurs. (default: false)
 * @param timeoutMs The timeout in milliseconds for waiting for an acknowledgement. (default: 200ms)
 * @return The status of the publish operation.
 */
rpcError ERPC::publish(uint8_t topicId, void* data, uint8_t length, bool requireAcknowledge, uint16_t timeoutMs)
{
    sendFrameStart();
    sendFrameInfo(requireAcknowledge, topicId);
    sendFrameLength(length);
    sendData(data, length);
    sendCrc();
    if (requireAcknowledge) {
        return waitForAcknowledge(timeoutMs);
    } else {
        return NO_ERROR;
    }
}

/**
 * @brief Get the index of a topic.
 *
 * @param topicId The ID of the topic.
 * @return The index of the topic if found, 0xFF if the topic was not found.
 */
uint8_t ERPC::getTopicIdx(uint8_t topicId)
{
    for (size_t idx = 0; idx < maxTopics_; idx++) {
        if (topics_[idx].topicId == topicId) {
            return idx;
        }
    }
    return 0xFF; // Return an invalid index if the topicId is not found
}

/**
 * @brief Send a byte to the serial port.
 *
 * @param byte The byte to be sent.
 * @param escape Whether the byte should be escaped.
 * @param crc Whether the byte should be included in the CRC calculation.
 */
void ERPC::serialByteWrite(uint8_t byte, bool escape, bool crc)
{
    if (escape && (byte == FRAME_START || byte == ESCAPE_CHARACTER)) {
        serialByteWriteRaw(ESCAPE_CHARACTER);
    }
    serialByteWriteRaw(byte);
    if (crc) {
        sendCrc16.add(byte);
    }
}

/**
 * @brief Send a byte to the serial port without handling the escaped character.
 * If the serial port is not available, the function will block until the port is available. (buffer is not full)
 *
 * @param byte The byte to be sent.
 */
void ERPC::serialByteWriteRaw(uint8_t byte)
{
    while (!serial->availableForWrite()) {
        delayMicroseconds(1);
    }
    serial->write(byte);
}
/**
 * @brief Read a byte from the serial port and handle the escaped character.
 *
 * @return The read byte.
 */
uint8_t ERPC::serialByteRead()
{
    uint8_t byte = serialByteReadRaw();
    if (byte == ESCAPE_CHARACTER) {
        byte = serialByteReadRaw(); // Read the next byte
    }
    return byte;
}

/**
 * @brief Read a byte from the serial port without handling the escaped character.
 * If not data is available, the function will block until data is available.
 * @return The read byte.
 */
uint8_t ERPC::serialByteReadRaw()
{
    while (!serial->available()) {
        delayMicroseconds(1);
    }
    return serial->read();
}

/**
 * @brief Send the frame start byte.
 */
void ERPC::sendFrameStart()
{
    sendCrc16.reset();
    serialByteWrite(FRAME_START, false, false);
}

/**
 * @brief Send the frame info byte. Structured according to the "FrameInfo" union.
 *
 * @param requireAcknowledge Whether an acknowledgement is required for the frame.
 * @param topicId The ID of the topic for the frame.
 */
void ERPC::sendFrameInfo(bool requireAcknowledge, uint8_t topicId)
{
    FrameInfo infoByte;
    infoByte.bits.topicId = topicId;
    infoByte.bits.isAck = 0;
    infoByte.bits.ackReq = requireAcknowledge;
    serialByteWrite(infoByte.byte, true, true);
}

/**
 * @brief Send the frame info byte for an acknowledgement frame.
 */
void ERPC::sendFrameInfoAck()
{
    FrameInfo infoByte;
    infoByte.bits.topicId = 63; // highest topicId value used for ACK
    infoByte.bits.isAck = 1;
    infoByte.bits.ackReq = 0;
    serialByteWrite(infoByte.byte, true, true);
}

/**
 * @brief Send the frame length byte.
 *
 * @param length The length of the frame to be sent.
 */
void ERPC::sendFrameLength(uint8_t length)
{
    serialByteWrite(length, true, true);
}

/**
 * @brief Send the data bytes.
 *
 * @param data A pointer to the data to be sent.
 * @param length The length of the data.
 */
void ERPC::sendData(void* data, size_t length)
{
    uint8_t* dataPtr = (uint8_t*)data;
    for (size_t i = 0; i < length; i++) {
        serialByteWrite(dataPtr[i], true, true);
    }
}

/**
 * @brief Send the CRC bytes.
 */
void ERPC::sendCrc()
{
    uint16_t crcValue = sendCrc16.calc();

    serialByteWrite((uint8_t)(crcValue >> 8), true, false);
    serialByteWrite((uint8_t)(crcValue & 0xFF), true, false);
}

/**
 * @brief Send an acknowledgement frame. This is used to acknowledge the receipt of a frame.
 * The frame type is set to ACK, the topic is set to an invalid value, and the data is set to the status of the received frame.
 */
void ERPC::sendAcknowledgeFrame()
{
    sendFrameStart();
    sendFrameInfoAck();
    sendFrameLength(1);
    serialByteWrite(rxStatus, true, true);
    sendCrc();
}

/**
 * @brief Handle the current state based on the received byte.
 * This is the main state machine that processes the incoming data from the serial port.
 *
 * @param byte The received byte.
 */
void ERPC::stateHandle(uint8_t byte)
{
    switch (currentState) {
    case IDLE:
        StateHandleIdle(byte);
        break;
    case RECEIVING_FRAMEINFO:
        StateHandleReceivingFrameInfo(byte);
        break;
    case RECEIVING_LENGTH:
        StateHandleReceivingLength(byte);
        break;
    case RECEIVING_DATA:
        StateHandleReceivingData(byte);
        break;
    case RECEIVING_CRC:
        StateHandleReceivingCrc(byte);
        break;
    default:
        break;
    }
}

/**
 * @brief Handle the IDLE state.
 *
 * @param byte The received byte.
 */
void ERPC::StateHandleIdle(uint8_t byte)
{
    if (byte == FRAME_START) {
        currentState = RECEIVING_FRAMEINFO;
        rxStatus = NO_ERROR;
        receiveCrc16.reset();
        if (receivedData != nullptr) {
            free(receivedData);
            receivedData = nullptr;
        }
    }
}

/**
 * @brief Handle the RECEIVING_FRAMEINFO state.
 *
 * @param byte The received byte.
 */
void ERPC::StateHandleReceivingFrameInfo(uint8_t byte)
{
    receiveCrc16.add(byte);
    receivedFrameInfo.byte = byte;

    if (receivedFrameInfo.bits.isAck == true) {
        currentState = RECEIVING_LENGTH; // topic is not used in ACK
    } else {
        currentTopicId = receivedFrameInfo.bits.topicId;
        currentTopicIdx = getTopicIdx(currentTopicId);
        if (currentTopicIdx == 0xFF) {
            rxStatus = NOT_SUBSCRIBED_ERROR;
            currentState = IDLE;
        } else {
            currentState = RECEIVING_LENGTH;
        }
    }
}

/**
 * @brief Handle the RECEIVING_LENGTH state.
 *
 * @param byte The received byte.
 */
void ERPC::StateHandleReceivingLength(uint8_t byte)
{
    receiveCrc16.add(byte);
    receivedDataLength = byte;
    receivedDataCounter = 0;

    receivedData = malloc(byte); // Allocate memory for the data
    memset(receivedData, 0, receivedDataLength);

    currentState = RECEIVING_DATA;
}

/**
 * @brief Handle the RECEIVING_DATA state.
 *
 * @param byte The received byte.
 */
void ERPC::StateHandleReceivingData(uint8_t byte)
{
    if (receivedData == nullptr) {
        currentState = IDLE;
        return;
    }

    receiveCrc16.add(byte);

    uint8_t* dataPtr = (uint8_t*)receivedData;
    dataPtr[receivedDataCounter] = byte;
    receivedDataCounter++;
    if (receivedDataCounter == receivedDataLength) {
        currentState = RECEIVING_CRC;
        receivedFirstCrcByte = false;
    }
}

/**
 * @brief Handle the RECEIVING_CRC state.
 * @details This state is used to validate the received frame and call the callback function for the current topic.
 * @details If the frame type is DATA_REQ_ACK, an acknowledgement frame is sent back.
 *
 * @param byte The received byte.
 */
void ERPC::StateHandleReceivingCrc(uint8_t byte)
{
    if (receivedFirstCrcByte == false) {
        receivedCrc = byte << 8;
        receivedFirstCrcByte = true;
        return;
    } else {
        receivedCrc |= byte;
    }
    bool crcValid = (receivedCrc == receiveCrc16.calc());

    if (crcValid) {
        rxStatus = NO_ERROR;

        if (receivedFrameInfo.bits.isAck) {
            receivedValidAck = true;
            u_int8_t* dataPtr = (uint8_t*)receivedData;
            receivedAckStatus = static_cast<rpcError>(dataPtr[0]); // Copy the status from the data
        } else {
            topicCallback();
            if (receivedFrameInfo.bits.ackReq) {
                sendAcknowledgeFrame();
            }
        }
    } else {
        if (receivedFrameInfo.bits.isAck == false) { // no callback for ACK
            rxStatus = CRC_ERROR;
            topicCallback();
        }
    }
    currentState = IDLE;
}

/**
 * @brief Wait for an acknowledgement after publishing data.
 *
 * @return The status of the acknowledgement.
 */
rpcError ERPC::waitForAcknowledge(uint16_t timeoutMs)
{
    unsigned long startTime = millis();
    receivedValidAck = false;
    receivedAckStatus = NO_ERROR;
    currentState = IDLE;

    while (receivedValidAck == false) {
        loop();
        if (millis() - startTime >= timeoutMs) {
            return ACK_TIMEOUT_ERROR;
        }
        delayMicroseconds(100);
    }
    return receivedAckStatus;
}

/**
 * @brief Call the callback function for the current topic.
 */
void ERPC::topicCallback()
{
    if (topics_[currentTopicIdx].callback != nullptr && receivedData != nullptr) { // we have a callback and data
        rpcError (*cb)(uint8_t, void* data, uint8_t, rpcError) = (rpcError(*)(uint8_t, void* data, uint8_t, rpcError))topics_[currentTopicIdx].callback;
        rxStatus = cb(currentTopicId, receivedData, receivedDataLength, rxStatus);
    }
}

