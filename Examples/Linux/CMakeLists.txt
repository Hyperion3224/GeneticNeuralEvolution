# 1. Build the Network Library (MasterServer, NodeClient, etc.)
add_library(NetworkLayer STATIC
    network/MasterServer.cpp
    network/NodeClient.cpp
)

# NetworkLayer needs the Core math/neural files and headers
target_link_libraries(NetworkLayer PUBLIC CoreSystems)
target_include_directories(NetworkLayer PUBLIC network network/net)

# 2. Build the Master Executable
# Depends on NetworkLayer (which brings in CoreSystems)
add_executable(Master Master.cpp)
target_link_libraries(Master PRIVATE NetworkLayer)

# 3. Build the Node Executable
# If Node uses the NetworkLayer classes (like NodeClient), link here:
add_executable(Node Node.cpp)
target_link_libraries(Node PRIVATE NetworkLayer)

# 4. Optional: Build Sockets as standalone examples (Unrelated to the above)
add_executable(SocketServer SocketServer.cpp)
add_executable(SocketClient SocketClient.cpp)