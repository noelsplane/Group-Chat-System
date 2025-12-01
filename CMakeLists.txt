cmake_minimum_required(VERSION 3.10)
project(GroupChatSystem)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/shared)

# Thread library
find_package(Threads REQUIRED)

# Server executable
add_executable(chat_server
    server/main.cpp
)

target_link_libraries(chat_server
    Threads::Threads
)

# Client executable
add_executable(chat_client
    client/main.cpp
)

target_link_libraries(chat_client
    Threads::Threads
)

# Compiler warnings
if(MSVC)
    target_compile_options(chat_server PRIVATE /W4)
    target_compile_options(chat_client PRIVATE /W4)
else()
    target_compile_options(chat_server PRIVATE -Wall -Wextra -pedantic)
    target_compile_options(chat_client PRIVATE -Wall -Wextra -pedantic)
endif()

# Installation
install(TARGETS chat_server chat_client
    RUNTIME DESTINATION bin
)
