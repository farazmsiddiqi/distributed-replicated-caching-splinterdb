cmake_minimum_required(VERSION 3.16)

project(ReplicatedSplinterDB
        VERSION 1.0
        DESCRIPTION "Replicating SplinterDB with Raft"
        LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

# Let's ensure -std=c++xx instead of -std=g++xx
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)

add_compile_options(
    -Wall -Wpedantic -Wextra -Wshadow -Wno-unused-parameter
    -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls 
    -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline
    -Wuninitialized -Wconversion -Wstrict-prototypes -Wno-gcc-compat
    -felide-constructors
)

# Import library shared objects and include directories
link_directories("${CMAKE_SOURCE_DIR}/libs")
include_directories("${CMAKE_SOURCE_DIR}/nuraft/include/")
include_directories("${CMAKE_SOURCE_DIR}/splinterdb/include/")
include_directories("${CMAKE_SOURCE_DIR}/rpclib/include/")

# Include executable subdirectories
add_subdirectory(apps)
add_subdirectory(src)
include_directories(include)