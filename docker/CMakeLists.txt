cmake_minimum_required(VERSION 3.7)

project(ReplicatedSplinterDB
        VERSION 1.0
        DESCRIPTION "Replicating SplinterDB with Raft"
        LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

# Let's ensure -std=c++xx instead of -std=g++xx
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)

# Import library shared objects and include directories
link_directories("${CMAKE_SOURCE_DIR}/libs")
link_directories("${CMAKE_SOURCE_DIR}/build/nuraft")

# Include directories
include_directories(include)
include_directories("${CMAKE_SOURCE_DIR}/nuraft/include/")
include_directories("${CMAKE_SOURCE_DIR}/rpclib/include/")
include_directories("${CMAKE_SOURCE_DIR}/splinterdb/include/")

# Include subdirectories with library source code 
set(DISABLE_SSL 1)
add_subdirectory(nuraft)
add_subdirectory(rpclib)
add_subdirectory(apps)
add_subdirectory(src)

set_property(DIRECTORY nuraft/tests PROPERTY EXCLUDE_FROM_ALL True)
set_property(DIRECTORY nuraft/examples PROPERTY EXCLUDE_FROM_ALL True)