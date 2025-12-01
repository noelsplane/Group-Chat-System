#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

// Message types
enum MessageType : uint8_t {
    MSG_TEXT = 1,
    MSG_JOIN_GROUP = 2,
    MSG_LEAVE_GROUP = 3,
    MSG_CREATE_GROUP = 4,
    MSG_LIST_GROUPS = 5,
    MSG_HISTORY = 6,
    MSG_AUDIO = 7,
    MSG_VIDEO = 8,
    MSG_ACK = 9,
    MSG_ERROR = 10
};

// Binary packet structure
#pragma pack(push, 1)
struct ChatPacket {
    uint8_t type;           // Message type
    uint16_t groupID;       // Group identifier
    uint32_t timestamp;     // Unix timestamp
    uint32_t senderID;      // Client identifier
    uint16_t payloadSize;   // Size of payload
    char payload[256];      // Message content
    
    ChatPacket() : type(0), groupID(0), timestamp(0), senderID(0), payloadSize(0) {
        memset(payload, 0, sizeof(payload));
    }
    
    // Convert to network byte order
    void toNetworkOrder() {
        groupID = htons(groupID);
        timestamp = htonl(timestamp);
        senderID = htonl(senderID);
        payloadSize = htons(payloadSize);
    }
    
    // Convert from network byte order
    void toHostOrder() {
        groupID = ntohs(groupID);
        timestamp = ntohl(timestamp);
        senderID = ntohl(senderID);
        payloadSize = ntohs(payloadSize);
    }
};
#pragma pack(pop)

#endif // PROTOCOL_H
