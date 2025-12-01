#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include "../shared/protocol.h"
#include "../shared/cache.h"
#include "../shared/utils.h"
#include "thread_pool.cpp"
#include "group_manager.cpp"

// Global objects
LRUCache messageCache(200);
GroupManager groupManager;
Logger serverLogger("../logs/server_log.txt");
ThreadPool* threadPool;
int server_fd;

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    serverLogger.log("Interrupt signal received. Shutting down server...");
    close(server_fd);
    delete threadPool;
    exit(signum);
}

void handleClient(int client_socket, uint32_t clientID, const std::string& clientIP) {
    serverLogger.log("New client connected", clientID, clientIP);
    
    char buffer[sizeof(ChatPacket)];
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(client_socket, buffer, sizeof(ChatPacket), 0);
        
        if (bytesRead <= 0) {
            serverLogger.log("Client disconnected", clientID, clientIP);
            groupManager.leaveGroup(clientID);
            break;
        }
        
        ChatPacket packet;
        memcpy(&packet, buffer, sizeof(ChatPacket));
        packet.toHostOrder();
        
        ChatPacket response;
        response.senderID = 0; // Server ID
        response.timestamp = getCurrentTimestamp();
        
        switch (packet.type) {
            case MSG_JOIN_GROUP: {
                uint16_t groupID = packet.groupID;
                if (groupManager.joinGroup(clientID, groupID)) {
                    response.type = MSG_ACK;
                    snprintf(response.payload, sizeof(response.payload), 
                            "Joined group %d", groupID);
                    serverLogger.log("Client joined group " + std::to_string(groupID), 
                                   clientID, clientIP);
                    
                    // Send recent message history
                    auto history = messageCache.getGroupHistory(groupID, 10);
                    for (const auto& msg : history) {
                        ChatPacket histMsg = msg;
                        histMsg.toNetworkOrder();
                        send(client_socket, &histMsg, sizeof(ChatPacket), 0);
                    }
                } else {
                    response.type = MSG_ERROR;
                    snprintf(response.payload, sizeof(response.payload), 
                            "Failed to join group %d", groupID);
                }
                break;
            }
            
            case MSG_CREATE_GROUP: {
                std::string groupName(packet.payload);
                uint16_t newGroupID = groupManager.createGroup(groupName);
                response.type = MSG_ACK;
                response.groupID = newGroupID;
                snprintf(response.payload, sizeof(response.payload), 
                        "Created group '%s' with ID %d", groupName.c_str(), newGroupID);
                serverLogger.log("Client created group: " + groupName, clientID, clientIP);
                break;
            }
            
            case MSG_LIST_GROUPS: {
                auto groups = groupManager.listGroups();
                response.type = MSG_ACK;
                std::string groupList;
                for (const auto& group : groups) {
                    groupList += std::to_string(group.first) + ":" + group.second + ";";
                }
                snprintf(response.payload, sizeof(response.payload), "%s", groupList.c_str());
                break;
            }
            
            case MSG_TEXT: {
                packet.senderID = clientID;
                packet.timestamp = getCurrentTimestamp();
                
                // Cache the message
                messageCache.put(packet);
                
                // Broadcast to all group members
                auto members = groupManager.getGroupMembers(packet.groupID);
                for (uint32_t memberID : members) {
                    // In a real implementation, maintain socket map for each client
                    // For now, just log
                    serverLogger.log("Broadcasting message to client " + 
                                   std::to_string(memberID), clientID, clientIP);
                }
                
                response.type = MSG_ACK;
                snprintf(response.payload, sizeof(response.payload), "Message sent");
                
                serverLogger.log("Message received for group " + 
                               std::to_string(packet.groupID) + ": " + 
                               std::string(packet.payload), clientID, clientIP);
                break;
            }
            
            case MSG_LEAVE_GROUP: {
                groupManager.leaveGroup(clientID);
                response.type = MSG_ACK;
                snprintf(response.payload, sizeof(response.payload), "Left group");
                serverLogger.log("Client left group", clientID, clientIP);
                break;
            }
            
            default:
                response.type = MSG_ERROR;
                snprintf(response.payload, sizeof(response.payload), "Unknown message type");
                break;
        }
        
        response.payloadSize = strlen(response.payload);
        response.toNetworkOrder();
        send(client_socket, &response, sizeof(ChatPacket), 0);
    }
    
    close(client_socket);
}

int main(int argc, char* argv[]) {
    // Setup signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    // Determine scheduling policy
    SchedulingPolicy policy = ROUND_ROBIN;
    if (argc > 2 && strcmp(argv[2], "sjf") == 0) {
        policy = SHORTEST_JOB_FIRST;
        serverLogger.log("Using Shortest Job First scheduling");
    } else {
        serverLogger.log("Using Round Robin scheduling");
    }
    
    threadPool = new ThreadPool(4, policy);
    
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Setsockopt failed" << std::endl;
        return -1;
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }
    
    // Listen
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }
    
    serverLogger.log("Server listening on port " + std::to_string(port));
    std::cout << "Chat server running on port " << port << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    uint32_t clientCounter = 1;
    
    while (true) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, 
                                   (socklen_t*)&addrlen);
        
        if (client_socket < 0) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }
        
        uint32_t clientID = clientCounter++;
        std::string clientIP = "client_ip"; // In real impl, extract from sockaddr
        
        // Enqueue client handling task
        threadPool->enqueue([client_socket, clientID, clientIP]() {
            handleClient(client_socket, clientID, clientIP);
        }, 10, clientID);
    }
    
    delete threadPool;
    close(server_fd);
    
    // Print statistics
    uint64_t processed, avgTime, hits, misses, evictions;
    threadPool->getStats(processed, avgTime);
    messageCache.getStats(hits, misses, evictions);
    
    std::cout << "\n=== Server Statistics ===" << std::endl;
    std::cout << "Tasks processed: " << processed << std::endl;
    std::cout << "Avg task time: " << avgTime << " μs" << std::endl;
    std::cout << "Cache hits: " << hits << std::endl;
    std::cout << "Cache misses: " << misses << std::endl;
    std::cout << "Cache evictions: " << evictions << std::endl;
    
    return 0;
}
