#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include "../shared/protocol.h"
#include "../shared/utils.h"

int sock = 0;
bool running = true;
Logger clientLogger("../logs/client_log.txt");

void receiveMessages() {
    char buffer[sizeof(ChatPacket)];
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(sock, buffer, sizeof(ChatPacket), 0);
        
        if (bytesRead <= 0) {
            std::cout << "\nDisconnected from server" << std::endl;
            running = false;
            break;
        }
        
        ChatPacket packet;
        memcpy(&packet, buffer, sizeof(ChatPacket));
        packet.toHostOrder();
        
        switch (packet.type) {
            case MSG_TEXT:
                std::cout << "\n[Group " << packet.groupID << "] "
                         << "[User " << packet.senderID << "] "
                         << formatTimestamp(packet.timestamp) << ": "
                         << packet.payload << std::endl;
                break;
            
            case MSG_ACK:
                std::cout << "\n[Server]: " << packet.payload << std::endl;
                break;
            
            case MSG_ERROR:
                std::cout << "\n[Error]: " << packet.payload << std::endl;
                break;
            
            case MSG_HISTORY:
                std::cout << "\n[History] [User " << packet.senderID << "] "
                         << formatTimestamp(packet.timestamp) << ": "
                         << packet.payload << std::endl;
                break;
            
            default:
                break;
        }
        
        std::cout << "> " << std::flush;
    }
}

void sendPacket(const ChatPacket& packet) {
    ChatPacket sendPkt = packet;
    sendPkt.toNetworkOrder();
    send(sock, &sendPkt, sizeof(ChatPacket), 0);
}

void printHelp() {
    std::cout << "\n=== Chat Client Commands ===" << std::endl;
    std::cout << "/join <group_id>     - Join a group" << std::endl;
    std::cout << "/create <group_name> - Create a new group" << std::endl;
    std::cout << "/list                - List all groups" << std::endl;
    std::cout << "/leave               - Leave current group" << std::endl;
    std::cout << "/help                - Show this help" << std::endl;
    std::cout << "/quit                - Quit the client" << std::endl;
    std::cout << "Type any message to send to current group" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string serverIP = "127.0.0.1";
    int port = 8080;
    
    if (argc > 1) {
        serverIP = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    
    struct sockaddr_in serv_addr;
    
    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    
    // Convert IPv4 address
    if (inet_pton(AF_INET, serverIP.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return -1;
    }
    
    // Connect
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection failed" << std::endl;
        return -1;
    }
    
    std::cout << "Connected to chat server at " << serverIP << ":" << port << std::endl;
    clientLogger.log("Connected to server at " + serverIP);
    
    printHelp();
    
    // Start receive thread
    std::thread recvThread(receiveMessages);
    recvThread.detach();
    
    uint16_t currentGroup = 0;
    
    while (running) {
        std::cout << "> ";
        std::string input;
        std::getline(std::cin, input);
        
        if (input.empty()) continue;
        
        ChatPacket packet;
        packet.timestamp = getCurrentTimestamp();
        
        if (input[0] == '/') {
            // Command
            if (input.find("/join ") == 0) {
                try {
                    uint16_t groupID = std::stoi(input.substr(6));
                    packet.type = MSG_JOIN_GROUP;
                    packet.groupID = groupID;
                    currentGroup = groupID;
                    sendPacket(packet);
                    clientLogger.log("Joining group " + std::to_string(groupID));
                } catch (...) {
                    std::cout << "Usage: /join <group_id>" << std::endl;
                }
            }
            else if (input.find("/create ") == 0) {
                std::string groupName = input.substr(8);
                packet.type = MSG_CREATE_GROUP;
                strncpy(packet.payload, groupName.c_str(), sizeof(packet.payload) - 1);
                packet.payloadSize = groupName.length();
                sendPacket(packet);
                clientLogger.log("Creating group: " + groupName);
            }
            else if (input == "/list") {
                packet.type = MSG_LIST_GROUPS;
                sendPacket(packet);
            }
            else if (input == "/leave") {
                packet.type = MSG_LEAVE_GROUP;
                sendPacket(packet);
                currentGroup = 0;
                clientLogger.log("Leaving group");
            }
            else if (input == "/help") {
                printHelp();
            }
            else if (input == "/quit") {
                running = false;
                clientLogger.log("Disconnecting from server");
                break;
            }
            else {
                std::cout << "Unknown command. Type /help for commands." << std::endl;
            }
        }
        else {
            // Regular message
            if (currentGroup == 0) {
                std::cout << "You must join a group first. Use /join <group_id>" << std::endl;
                continue;
            }
            
            packet.type = MSG_TEXT;
            packet.groupID = currentGroup;
            strncpy(packet.payload, input.c_str(), sizeof(packet.payload) - 1);
            packet.payloadSize = input.length();
            sendPacket(packet);
            
            clientLogger.log("Sent message to group " + std::to_string(currentGroup) + 
                           ": " + input);
        }
    }
    
    close(sock);
    std::cout << "Disconnected from server" << std::endl;
    
    return 0;
}
