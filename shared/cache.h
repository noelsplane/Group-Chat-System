#ifndef CACHE_H
#define CACHE_H

#include <list>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>
#include <vector>
#include "protocol.h"

struct CacheEntry {
    ChatPacket message;
    std::chrono::time_point<std::chrono::system_clock> timestamp;
    uint32_t ttl; // Time-to-live in seconds
    
    CacheEntry(const ChatPacket& msg, uint32_t ttlSeconds = 3600) 
        : message(msg), 
          timestamp(std::chrono::system_clock::now()),
          ttl(ttlSeconds) {}
    
    bool isExpired() const {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - timestamp);
        return elapsed.count() > ttl;
    }
};

class LRUCache {
private:
    size_t capacity;
    std::list<std::shared_ptr<CacheEntry>> cacheList;
    std::unordered_map<uint64_t, std::list<std::shared_ptr<CacheEntry>>::iterator> cacheMap;
    std::mutex cacheMutex;
    
    // Statistics
    uint64_t hits;
    uint64_t misses;
    uint64_t evictions;
    
    uint64_t makeKey(uint16_t groupID, uint32_t timestamp) {
        return (static_cast<uint64_t>(groupID) << 32) | timestamp;
    }
    
public:
    LRUCache(size_t cap = 100) : capacity(cap), hits(0), misses(0), evictions(0) {}
    
    void put(const ChatPacket& packet, uint32_t ttl = 3600) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        uint64_t key = makeKey(packet.groupID, packet.timestamp);
        
        // Remove if already exists
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            cacheList.erase(it->second);
            cacheMap.erase(it);
        }
        
        // Add to front
        auto entry = std::make_shared<CacheEntry>(packet, ttl);
        cacheList.push_front(entry);
        cacheMap[key] = cacheList.begin();
        
        // Evict if over capacity
        if (cacheList.size() > capacity) {
            auto last = cacheList.back();
            uint64_t evictKey = makeKey(last->message.groupID, last->message.timestamp);
            cacheMap.erase(evictKey);
            cacheList.pop_back();
            evictions++;
        }
    }
    
    bool get(uint16_t groupID, uint32_t timestamp, ChatPacket& packet) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        
        uint64_t key = makeKey(groupID, timestamp);
        auto it = cacheMap.find(key);
        
        if (it == cacheMap.end()) {
            misses++;
            return false;
        }
        
        // Check TTL
        if ((*it->second)->isExpired()) {
            cacheList.erase(it->second);
            cacheMap.erase(it);
            misses++;
            return false;
        }
        
        // Move to front (most recently used)
        cacheList.splice(cacheList.begin(), cacheList, it->second);
        packet = (*it->second)->message;
        hits++;
        return true;
    }
    
    std::vector<ChatPacket> getGroupHistory(uint16_t groupID, size_t limit = 10) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        std::vector<ChatPacket> history;
        
        for (auto& entry : cacheList) {
            if (entry->message.groupID == groupID && !entry->isExpired()) {
                history.push_back(entry->message);
                if (history.size() >= limit) break;
            }
        }
        
        return history;
    }
    
    void getStats(uint64_t& h, uint64_t& m, uint64_t& e) {
        std::lock_guard<std::mutex> lock(cacheMutex);
        h = hits;
        m = misses;
        e = evictions;
    }
    
    void clearExpired() {
        std::lock_guard<std::mutex> lock(cacheMutex);
        auto it = cacheList.begin();
        while (it != cacheList.end()) {
            if ((*it)->isExpired()) {
                uint64_t key = makeKey((*it)->message.groupID, (*it)->message.timestamp);
                cacheMap.erase(key);
                it = cacheList.erase(it);
            } else {
                ++it;
            }
        }
    }
};

#endif // CACHE_H
