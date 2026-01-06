cmake_minimum_required(VERSION 4.2)
project(DistributedSystem)

# Order matters here for build priority
add_subdirectory(Libraries)

if(UNIX)
    add_subdirectory(Linux)
elseif(WIN32)
    add_subdirectory(Windows)
endif()