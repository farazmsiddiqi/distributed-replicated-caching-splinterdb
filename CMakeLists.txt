cmake_minimum_required(VERSION 3.7)

project(ReplicatedSplinterDB
        VERSION 1.0
        DESCRIPTION "Replicating SplinterDB with Raft"
        LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

# Let's ensure -std=c++xx instead of -std=g++xx
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)

# Import library shared objects and include directories
link_directories("${ReplicatedSplinterDB_SOURCE_DIR}/libs")
link_directories("${ReplicatedSplinterDB_SOURCE_DIR}/build/nuraft")

# Include directories
include_directories("${ReplicatedSplinterDB_SOURCE_DIR}/include")
include_directories("${ReplicatedSplinterDB_SOURCE_DIR}/third-party/nuraft/include/")
include_directories("${ReplicatedSplinterDB_SOURCE_DIR}/third-party/rpclib/include/")
include_directories("${ReplicatedSplinterDB_SOURCE_DIR}/third-party/splinterdb/include/")

# Include subdirectories with library source code 
set(DISABLE_SSL 1)
add_subdirectory("${ReplicatedSplinterDB_SOURCE_DIR}/third-party/nuraft" EXCLUDE_FROM_ALL)
add_subdirectory("${ReplicatedSplinterDB_SOURCE_DIR}/third-party/rpclib" EXCLUDE_FROM_ALL)
add_subdirectory("${ReplicatedSplinterDB_SOURCE_DIR}/apps" EXCLUDE_FROM_ALL)
add_subdirectory("${ReplicatedSplinterDB_SOURCE_DIR}/src/client" EXCLUDE_FROM_ALL)
add_subdirectory("${ReplicatedSplinterDB_SOURCE_DIR}/src/server" EXCLUDE_FROM_ALL)

set_property(DIRECTORY "${ReplicatedSplinterDB_SOURCE_DIR}/third-party/nuraft/tests" PROPERTY EXCLUDE_FROM_ALL True)
set_property(DIRECTORY "${ReplicatedSplinterDB_SOURCE_DIR}/third-party/nuraft/examples" PROPERTY EXCLUDE_FROM_ALL True)