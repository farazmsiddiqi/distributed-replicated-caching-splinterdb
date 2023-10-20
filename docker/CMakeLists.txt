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
add_subdirectory(apps EXCLUDE_FROM_ALL)
add_subdirectory(src)

set_property(DIRECTORY nuraft/tests PROPERTY EXCLUDE_FROM_ALL True)
set_property(DIRECTORY nuraft/examples PROPERTY EXCLUDE_FROM_ALL True)

# Define the replicated-splinterdb-* libraries
file(GLOB SERVER_HEADER_LIST CONFIGURE_DEPENDS
    "${ReplicatedSplinterDB_SOURCE_DIR}/include/server/*.h"
    "${ReplicatedSplinterDB_SOURCE_DIR}/include/common/*.h")

file(GLOB CLIENT_HEADER_LIST CONFIGURE_DEPENDS
    "${ReplicatedSplinterDB_SOURCE_DIR}/include/client/*.h"
    "${ReplicatedSplinterDB_SOURCE_DIR}/include/common/*.h")

file(GLOB SERVER_SOURCE_LIST CONFIGURE_DEPENDS
    "${ReplicatedSplinterDB_SOURCE_DIR}/src/server/*.cpp")

file(GLOB CLIENT_SOURCE_LIST CONFIGURE_DEPENDS
    "${ReplicatedSplinterDB_SOURCE_DIR}/src/client/*.cpp")

# Make the static libraries that are to be linked to the main executables
add_library(replicated-splinterdb-server STATIC ${SERVER_SOURCE_LIST} ${SERVER_HEADER_LIST})
add_library(replicated-splinterdb-client STATIC ${CLIENT_SOURCE_LIST} ${CLIENT_HEADER_LIST})

# All users of these libraries will need at least C++20
target_compile_features(replicated-splinterdb-server PUBLIC cxx_std_20)
target_compile_features(replicated-splinterdb-client PUBLIC cxx_std_20)

# All users of the server library will need to define SPLINTERDB_PLATFORM_DIR
target_compile_definitions(replicated-splinterdb-server PUBLIC -DSPLINTERDB_PLATFORM_DIR=platform_linux)

# Add a bunch of warnings for the library build
target_compile_options(
    replicated-splinterdb-server PRIVATE
    -Wall -Wpedantic -Wextra -Wshadow -Wno-unused-parameter
    -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls 
    -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline
    -Wuninitialized -Wconversion -Wstrict-prototypes -felide-constructors
    -Wno-gcc-compat
)

target_compile_options(
    replicated-splinterdb-client PRIVATE
    -Wall -Wpedantic -Wextra -Wshadow -Wno-unused-parameter
    -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls 
    -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline
    -Wuninitialized -Wconversion -Wstrict-prototypes -felide-constructors
    -Wno-gcc-compat
)

# Users of our libraries will need these include directories
target_include_directories(replicated-splinterdb-server PUBLIC "${ReplicatedSplinterDB_SOURCE_DIR}/include/server/")
target_include_directories(replicated-splinterdb-client PUBLIC "${ReplicatedSplinterDB_SOURCE_DIR}/include/client/")

# Link the libraries to some other dependencies
target_link_libraries(replicated-splinterdb-server nuraft.a rpc splinterdb pthread)
target_link_libraries(replicated-splinterdb-client rpc pthread)