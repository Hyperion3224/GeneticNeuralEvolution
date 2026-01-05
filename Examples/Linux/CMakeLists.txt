cmake_minimum_required(VERSION 3.16)

project(DistNeuralNetLinux LANGUAGES CXX)

# add_executable(Master Master.cpp)
# add_executable(Node Node.cpp)
add_executable(SocketClient SocketClient.cpp)
add_executable(SocketServer SocketServer.cpp)