#ifndef ERPC_h
#define ERPC_h

#include <Arduino.h>
#include <CRC16.h>

#define FRAME_START 0x7E
#define ESCAPE_CHARACTER 0x7F

/**
 * @brief Enumeration representing the different error codes that can be returned by the ERPC protocol.
 */
enum rpcError {
    NO_ERROR,
    NOT_SUBSCRIBED_ERROR,
    CRC_ERROR,
    FRAME_TYPE_ERROR,
    ACK_TIMEOUT_ERROR,
    PROCESSING_ERROR
};

/**
 * @class ERPC
 * @brief A class for Serial RPC communication.
 *
 * This class provides methods for subscribing and unsubscribing to topics in a Serial RPC communication system.
 * Each topic has an associated callback function that is called when data is received on the topic.
 * The class also provides a method for publishing data on a topic.
 *
 * @note Protocol frame structure:
 * 1 byte start (0x7E)
 * 1 byte info (6 bits topicId, 1 bit isAck, 1 bit ackReq). little endian!
 * 1 byte length (0-255)
 * n bytes data (escaped with 0x7F if 0x7E or 0x7F is in the data field (or info, length field)
 * 2 bytes crc (CRC16-CCITT calculated over the info, length and data fields)
 */
class ERPC {

public:
    ERPC(HardwareSerial& serial, uint8_t maxTopics = 10);
    void loop();

    bool subscribe(uint8_t topicId, rpcError (*callback)(uint8_t, void*, uint8_t, rpcError));
    bool unsubscribe(uint8_t topicId);
    rpcError publish(uint8_t topicId, void* data, uint8_t length, bool reqiureAcknowledge = false, uint16_t timeoutMs = 200);

private:
    /**
     * @brief Enumeration representing the different states of the ERPC protocol.
     */
    enum SerRpcState {
        IDLE, /**< Idle state */
        RECEIVING_FRAMEINFO, /**< Receiving frame info state */
        RECEIVING_LENGTH, /**< Receiving length state */
        RECEIVING_DATA, /**< Receiving data state */
        RECEIVING_CRC /**< Receiving CRC state */
    };

    /**
     * @brief Structure representing a topic.
     */
    struct Topic {
        uint8_t topicId;
        rpcError (*callback)(uint8_t, void*, uint8_t, rpcError);
        bool used; // If true, the topic is subscribed
    };

    // Frame info structure (sent in the info byte of the frame)
    union FrameInfo { // Used for bit manipulation of the info byte
        struct Bits {
            uint8_t topicId : 6; // 6 bits for topicId
            uint8_t isAck : 1; // 1 bit for isAck
            uint8_t ackReq : 1; // 1 bit for ackReq
        } bits;
        uint8_t byte;
    };

    // Other class instances
    CRC16 sendCrc16;
    CRC16 receiveCrc16;
    HardwareSerial* serial;

    // Topic handling
    uint8_t maxTopics_; // How much memory to allocate for topics. aka number of topics
    Topic* topics_; // Array of topics
    uint8_t currentTopicIdx; // When receiving data, this is the index of the current topic (not the topicId!)
    uint8_t currentTopicId; // When receiving data, this is the topicId of the current topic
    uint8_t getTopicIdx(uint8_t topicId);

    // variables for receiving data
    void* receivedData = nullptr;
    uint8_t receivedDataCounter;
    uint8_t receivedDataLength;
    FrameInfo receivedFrameInfo;
    uint16_t receivedCrc; // received crc
    bool receivedFirstCrcByte; // true if received first crc byte

    // received status
    SerRpcState currentState = IDLE; // machine state
    rpcError rxStatus = NO_ERROR;
    bool receivedValidAck = false; // set true if received ack has valid crc
    rpcError receivedAckStatus = NO_ERROR;

    // Serial communication
    void serialByteWrite(uint8_t byte, bool escape, bool crc);
    void serialByteWriteRaw(uint8_t byte);
    uint8_t serialByteRead();
    uint8_t serialByteReadRaw();

    // sending frames
    void sendFrameStart();
    void sendFrameInfo(bool requireAcknowledge, uint8_t topicId);
    void sendFrameInfoAck();
    void sendFrameLength(uint8_t length);
    void sendData(void* data, size_t length);
    void sendCrc();
    void sendAcknowledgeFrame();

    // receiving frames (state machine)
    void stateHandle(uint8_t byte);
    void StateHandleIdle(uint8_t byte);
    void StateHandleReceivingFrameInfo(uint8_t byte);
    void StateHandleReceivingLength(uint8_t byte);
    void StateHandleReceivingData(uint8_t byte);
    void StateHandleReceivingCrc(uint8_t byte);
    rpcError waitForAcknowledge(uint16_t timeoutMs);
    void topicCallback();
};

#endif