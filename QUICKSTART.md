# Quick Start Guide
## Operating Systems Final Project - C++ Group Chat System

## What Has Been Created

Your complete Operating Systems final project has been set up with:

### ✅ Project Structure
```
GroupChat/
├── client/          - Chat client with CLI interface
├── server/          - Multi-threaded chat server
├── shared/          - Protocol definitions, cache, utilities
├── logs/            - Log file directory
├── diagrams/        - For architecture diagrams
├── tests/           - Test files
├── CMakeLists.txt   - Build configuration
└── README.md        - Full documentation
```

### ✅ Core Components Implemented

1. **Binary Protocol** (`shared/protocol.h`)
   - ChatPacket structure with 10 message types
   - Network byte order conversion

2. **LRU Cache** (`shared/cache.h`)
   - Least Recently Used eviction policy
   - TTL (time-to-live) support
   - Thread-safe with statistics tracking

3. **Thread Pool** (`server/thread_pool.cpp`)
   - Round Robin scheduling
   - Shortest Job First scheduling
   - Worker thread management

4. **Group Manager** (`server/group_manager.cpp`)
   - Multi-group support
   - Client membership tracking
   - Thread-safe group operations

5. **Server** (`server/main.cpp`)
   - TCP socket server
   - Signal handling (SIGINT, SIGTERM)
   - Client connection management
   - Message broadcasting

6. **Client** (`client/main.cpp`)
   - Async message receiving
   - Command-line interface
   - Commands: /join, /create, /list, /leave, /quit

7. **Logging System** (`shared/utils.h`)
   - Timestamped logging
   - User ID and IP tracking

## Next Steps

### 1. Build the Project (Linux/macOS/WSL)

**Note**: The project uses POSIX sockets which work on Linux/macOS/WSL but need modification for Windows.

```bash
# Using CMake (recommended)
mkdir build
cd build
cmake ..
make

# Or compile manually
g++ -std=c++17 -pthread server/main.cpp -o chat_server
g++ -std=c++17 -pthread client/main.cpp -o chat_client
```

### 2. Run the System

**Terminal 1 - Start Server:**
```bash
./chat_server
# Or with custom port and SJF scheduling:
./chat_server 8080 sjf
```

**Terminal 2 - Start Client 1:**
```bash
./chat_client
> /list
> /join 1
> Hello!
```

**Terminal 3 - Start Client 2:**
```bash
./chat_client
> /join 1
> Hi there!
```

### 3. Test All Features

- [ ] Create a new group: `/create MyGroup`
- [ ] List groups: `/list`
- [ ] Join a group: `/join 1`
- [ ] Send messages
- [ ] Switch groups
- [ ] Join as new client (should receive history)
- [ ] Check logs in `logs/` directory
- [ ] Test graceful shutdown (Ctrl+C on server)

### 4. Generate Performance Data

Run the server with multiple clients sending messages to collect:
- Tasks processed
- Average task time
- Cache hit/miss ratio
- Cache evictions

Statistics are displayed when the server shuts down.


### 7. Windows Compatibility (If Needed)

The current code uses POSIX sockets. For Windows:

**Option A**: Use WSL (Windows Subsystem for Linux)
- Easiest option, no code changes needed

**Option B**: Modify for Winsock
- Replace `#include <sys/socket.h>` with `#include <winsock2.h>`
- Add `#pragma comment(lib, "ws2_32.lib")`
- Initialize Winsock with WSAStartup()
- Replace close() with closesocket()

## OS Concepts Checklist

✅ **Threads/Processes**: Thread pool with 4 worker threads  
✅ **Synchronization**: Mutexes, condition variables, deadlock prevention  
✅ **Scheduling**: Round Robin and Shortest Job First  
✅ **Socket Programming**: TCP client-server communication  
✅ **Memory Management**: Smart pointers, LRU cache  
✅ **Caching**: LRU with TTL and statistics  
✅ **File I/O**: Logging system  
✅ **Binary Protocol**: ChatPacket with network byte order  
✅ **Interrupts**: Signal handling for graceful shutdown  


## Resources

- Full documentation: `README.md`
- Presentation prompt: `MANUS_PRESENTATION_PROMPT.txt`
- Server logs: `logs/server_log.txt`
- Client logs: `logs/client_log.txt`

## Need Help?

1. Check the README.md for detailed documentation
2. Review the code comments in each file
3. Test each component individually
4. Use the logging system to debug issues

Good luck with your final project presentation! 🚀
