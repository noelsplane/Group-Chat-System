#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <mutex>

class Logger {
private:
    std::ofstream logFile;
    std::mutex logMutex;
    
public:
    Logger(const std::string& filename) {
        logFile.open(filename, std::ios::app);
    }
    
    ~Logger() {
        if (logFile.is_open()) {
            logFile.close();
        }
    }
    
    void log(const std::string& message, uint32_t userID = 0, const std::string& ipAddress = "") {
        std::lock_guard<std::mutex> lock(logMutex);
        
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        
        std::stringstream ss;
        ss << "[" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S") << "] ";
        
        if (userID != 0) {
            ss << "UserID:" << userID << " ";
        }
        
        if (!ipAddress.empty()) {
            ss << "IP:" << ipAddress << " ";
        }
        
        ss << message << std::endl;
        
        if (logFile.is_open()) {
            logFile << ss.str();
            logFile.flush();
        }
    }
};

inline uint32_t getCurrentTimestamp() {
    return static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

inline std::string formatTimestamp(uint32_t timestamp) {
    time_t t = static_cast<time_t>(timestamp);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

#endif // UTILS_H
