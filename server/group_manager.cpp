#ifndef GROUP_MANAGER_H
#define GROUP_MANAGER_H

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <string>
#include "../shared/protocol.h"

struct ChatGroup {
    uint16_t groupID;
    std::string groupName;
    std::unordered_set<uint32_t> members;
    std::mutex groupMutex;
    
    ChatGroup(uint16_t id, const std::string& name) 
        : groupID(id), groupName(name) {}
    
    void addMember(uint32_t clientID) {
        std::lock_guard<std::mutex> lock(groupMutex);
        members.insert(clientID);
    }
    
    void removeMember(uint32_t clientID) {
        std::lock_guard<std::mutex> lock(groupMutex);
        members.erase(clientID);
    }
    
    std::vector<uint32_t> getMembers() {
        std::lock_guard<std::mutex> lock(groupMutex);
        return std::vector<uint32_t>(members.begin(), members.end());
    }
    
    size_t getMemberCount() {
        std::lock_guard<std::mutex> lock(groupMutex);
        return members.size();
    }
};

class GroupManager {
private:
    std::unordered_map<uint16_t, std::shared_ptr<ChatGroup>> groups;
    std::unordered_map<uint32_t, uint16_t> clientGroups; // client -> current group
    std::mutex managerMutex;
    uint16_t nextGroupID;
    
public:
    GroupManager() : nextGroupID(1) {
        // Create default group
        createGroup("General");
    }
    
    uint16_t createGroup(const std::string& name) {
        std::lock_guard<std::mutex> lock(managerMutex);
        uint16_t groupID = nextGroupID++;
        groups[groupID] = std::make_shared<ChatGroup>(groupID, name);
        return groupID;
    }
    
    bool joinGroup(uint32_t clientID, uint16_t groupID) {
        std::lock_guard<std::mutex> lock(managerMutex);
        
        auto it = groups.find(groupID);
        if (it == groups.end()) {
            return false;
        }
        
        // Leave current group if any
        auto currentIt = clientGroups.find(clientID);
        if (currentIt != clientGroups.end()) {
            auto currentGroup = groups.find(currentIt->second);
            if (currentGroup != groups.end()) {
                currentGroup->second->removeMember(clientID);
            }
        }
        
        // Join new group
        it->second->addMember(clientID);
        clientGroups[clientID] = groupID;
        return true;
    }
    
    void leaveGroup(uint32_t clientID) {
        std::lock_guard<std::mutex> lock(managerMutex);
        
        auto it = clientGroups.find(clientID);
        if (it != clientGroups.end()) {
            auto group = groups.find(it->second);
            if (group != groups.end()) {
                group->second->removeMember(clientID);
            }
            clientGroups.erase(it);
        }
    }
    
    std::vector<uint32_t> getGroupMembers(uint16_t groupID) {
        std::lock_guard<std::mutex> lock(managerMutex);
        
        auto it = groups.find(groupID);
        if (it != groups.end()) {
            return it->second->getMembers();
        }
        return {};
    }
    
    std::vector<std::pair<uint16_t, std::string>> listGroups() {
        std::lock_guard<std::mutex> lock(managerMutex);
        
        std::vector<std::pair<uint16_t, std::string>> result;
        for (const auto& pair : groups) {
            result.emplace_back(pair.first, pair.second->groupName);
        }
        return result;
    }
    
    uint16_t getClientGroup(uint32_t clientID) {
        std::lock_guard<std::mutex> lock(managerMutex);
        
        auto it = clientGroups.find(clientID);
        if (it != clientGroups.end()) {
            return it->second;
        }
        return 0; // Not in any group
    }
};

#endif // GROUP_MANAGER_H
