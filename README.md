# C++ Group Chat System
## Operating Systems Final Project

A multi-group, client-server chat system implemented in C++ featuring text messaging, LRU caching, thread pool scheduling, and comprehensive OS concepts integration.

## Features

### Core Features (Required)
- **Multi-Group Text Messaging**: Clients can join, create, and switch between multiple chat groups
- **Message Broadcasting**: Messages are broadcast to all members in a group
- **Message History**: New users receive the latest 10 messages when joining a group
- **LRU Caching**: Recent messages cached with TTL (time-to-live) expiration
- **Thread Pool Scheduling**: Support for both Round Robin and Shortest Job First scheduling
- **Binary Protocol**: Custom packet structure with network byte order conversion
- **Logging System**: Comprehensive logging of all actions with timestamps and user IDs
- **Synchronization**: Mutex-protected shared resources with deadlock prevention

### OS Concepts Implemented

| Concept | Implementation |
|---------|----------------|
| **Threads/Concurrency** | Thread pool with worker threads |
| **Synchronization** | Mutex, condition variables for queues and caches |
| **Scheduling** | Round Robin and Shortest Job First algorithms |
| **Socket Programming** | TCP sockets for client-server communication |
| **Memory Management** | LRU cache with eviction policy, smart pointers |
| **Caching** | LRU cache with TTL and statistics tracking |
| **File I/O** | Logging system with file-based storage |
| **Binary Protocol** | Structured packet with network byte order |
| **Interrupts** | Signal handling for graceful shutdown |
| **Deadlock Prevention** | Resource ordering with mutex locks |

## Project Structure

```
GroupChat/
├── client/
│   └── main.cpp                    # Chat client implementation
├── server/
│   ├── main.cpp                    # Server with client handling
│   ├── thread_pool.cpp             # Thread pool with RR/SJF scheduling
│   └── group_manager.cpp           # Group management logic
├── shared/
│   ├── protocol.h                  # Binary packet structure
│   ├── cache.h                     # LRU cache implementation
│   └── utils.h                     # Logger and utility functions
├── logs/
│   ├── server_log.txt              # Server logs
│   └── client_log.txt              # Client logs
├── diagrams/                       # Architecture diagrams
├── tests/                          # Test files
├── CMakeLists.txt                  # Build configuration
└── README.md                       # This file
```

## Building the Project

### Prerequisites
- C++17 compatible compiler (g++, clang++)
- CMake 3.10 or higher
- pthread library

### Compilation

#### Using CMake (Recommended)
```bash
mkdir build
cd build
cmake ..
make
```

#### Manual Compilation (Linux/macOS)
```bash
# Server
g++ -std=c++17 -pthread server/main.cpp -o chat_server

# Client
g++ -std=c++17 -pthread client/main.cpp -o chat_client
```

#### For Windows (MinGW)
```bash
g++ -std=c++17 server/main.cpp -o chat_server.exe -lws2_32
g++ -std=c++17 client/main.cpp -o chat_client.exe -lws2_32
```

## Running the System

### Start the Server
```bash
# Default (port 8080, Round Robin scheduling)
./chat_server

# Custom port
./chat_server 9000

# With Shortest Job First scheduling
./chat_server 8080 sjf
```

### Start the Client
```bash
# Default (localhost:8080)
./chat_client

# Custom server and port
./chat_client 192.168.1.100 8080
```

## Client Commands

| Command | Description |
|---------|-------------|
| `/join <group_id>` | Join an existing group |
| `/create <group_name>` | Create a new group |
| `/list` | List all available groups |
| `/leave` | Leave current group |
| `/help` | Show help message |
| `/quit` | Disconnect from server |
| `<message>` | Send message to current group |

## Usage Example

### Server Terminal
```
$ ./chat_server
Chat server running on port 8080
Press Ctrl+C to stop
```

### Client 1 Terminal
```
$ ./chat_client
Connected to chat server at 127.0.0.1:8080

> /list
[Server]: 1:General;

> /join 1
[Server]: Joined group 1

> Hello everyone!
```

### Client 2 Terminal
```
$ ./chat_client
Connected to chat server at 127.0.0.1:8080

> /join 1
[Server]: Joined group 1
[History] [User 1] 2024-12-01 14:30:15: Hello everyone!

> Hi there!
```

## Binary Protocol Specification

```cpp
struct ChatPacket {
    uint8_t type;           // Message type (1-10)
    uint16_t groupID;       // Group identifier
    uint32_t timestamp;     // Unix timestamp
    uint32_t senderID;      // Client identifier
    uint16_t payloadSize;   // Size of payload
    char payload[256];      // Message content
};
```

**Message Types:**
- 1: TEXT - Regular text message
- 2: JOIN_GROUP - Join group request
- 3: LEAVE_GROUP - Leave group request
- 4: CREATE_GROUP - Create new group
- 5: LIST_GROUPS - List all groups
- 6: HISTORY - Message history
- 7: AUDIO - Audio data (optional)
- 8: VIDEO - Video data (optional)
- 9: ACK - Acknowledgment
- 10: ERROR - Error message

## Architecture

### Thread Pool Design
- Configurable number of worker threads (default: 4)
- Two scheduling policies:
  - **Round Robin**: FIFO task queue
  - **Shortest Job First**: Priority queue based on estimated task time
- Statistics tracking: tasks processed, average wait time

### Cache Design
- **Policy**: Least Recently Used (LRU)
- **Capacity**: 200 messages (configurable)
- **TTL**: 3600 seconds (1 hour, configurable)
- **Statistics**: Cache hits, misses, evictions
- Thread-safe with mutex protection

### Synchronization Strategy
- **Message Queue**: Protected by mutex + condition variable
- **Cache Access**: Mutex-protected with fine-grained locking
- **Group Management**: Per-group and manager-level mutexes
- **Deadlock Prevention**: Consistent lock ordering

## Performance Benchmarking

The server tracks and reports:
- Total tasks processed
- Average task processing time
- Cache hit/miss ratio
- Cache evictions
- Message throughput

Example output:
```
=== Server Statistics ===
Tasks processed: 1523
Avg task time: 245 μs
Cache hits: 892
Cache misses: 156
Cache evictions: 23
```

## Logging

### Server Logs (`logs/server_log.txt`)
```
[2024-12-01 14:30:12] Server listening on port 8080
[2024-12-01 14:30:15] UserID:1 IP:client_ip New client connected
[2024-12-01 14:30:18] UserID:1 IP:client_ip Client joined group 1
```

### Client Logs (`logs/client_log.txt`)
```
[2024-12-01 14:30:15] Connected to server at 127.0.0.1
[2024-12-01 14:30:18] Joining group 1
[2024-12-01 14:30:25] Sent message to group 1: Hello!
```

## Testing

### Manual Testing
1. Start the server
2. Connect multiple clients
3. Create groups and send messages
4. Verify message broadcasting
5. Test group switching
6. Verify message history on join

### Automated Testing (Future)
- Bot test harness for load testing
- Performance benchmarking scripts
- Wireshark packet capture analysis

## Known Limitations

1. **Broadcasting**: Current implementation logs broadcast but doesn't maintain socket map for actual broadcasting (simplified for demonstration)
2. **Authentication**: No user authentication or security features
3. **Persistence**: Messages not persisted to disk (only in-memory cache)
4. **Windows Compatibility**: Uses POSIX sockets (requires adaptation for Windows)

## Future Enhancements (Optional Features)

- [ ] Audio/Video streaming support
- [ ] File transfer capability
- [ ] End-to-end encryption
- [ ] User authentication and permissions
- [ ] Database integration for message persistence
- [ ] Web-based client interface
- [ ] Message acknowledgment and retry logic
- [ ] Compression for large messages

## OS Concepts Coverage

### Memory Management
- Smart pointers (std::shared_ptr, std::unique_ptr)
- Custom LRU cache with manual eviction
- Fixed-size buffers to simulate memory constraints

### Process Synchronization
- Mutexes for critical sections
- Condition variables for wait-notify patterns
- Deadlock prevention through consistent ordering

### Scheduling Algorithms
- Round Robin: Fair scheduling with FIFO queue
- Shortest Job First: Priority-based task selection

### File Systems
- Log file creation, writing, deletion
- Append-mode file operations
- Timestamped entries

## References

- POSIX Threads Programming: https://computing.llnl.gov/tutorials/pthreads/
- TCP/IP Sockets in C: Practical Guide
- Operating Systems: Three Easy Pieces
- C++ Concurrency in Action by Anthony Williams

## Authors

[Jayden Brown,
Brock Caston]
Operating Systems Course - Fall 2025


