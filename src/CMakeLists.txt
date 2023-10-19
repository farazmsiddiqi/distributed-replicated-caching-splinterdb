file(GLOB HEADER_LIST CONFIGURE_DEPENDS
        "${ReplicatedSplinterDB_SOURCE_DIR}/include/*.h"
        "${ReplicatedSplinterDB_SOURCE_DIR}/include/*.hpp"
        "${ReplicatedSplinterDB_SOURCE_DIR}/include/*.hxx")

file(GLOB SOURCE_LIST CONFIGURE_DEPENDS
        "${ReplicatedSplinterDB_SOURCE_DIR}/src/*.cc"
        "${ReplicatedSplinterDB_SOURCE_DIR}/src/*.cpp"
        "${ReplicatedSplinterDB_SOURCE_DIR}/src/*.cxx")

# Make an automatic library - will be static or dynamic based on user setting
add_library(replicated-splinterdb ${SOURCE_LIST} ${HEADER_LIST})

# All users of this library will need at least C++20
target_compile_features(replicated-splinterdb PUBLIC cxx_std_20)

# All users of this library will need to define SPLINTERDB_PLATFORM_DIR
target_compile_definitions(replicated-splinterdb PUBLIC -DSPLINTERDB_PLATFORM_DIR=platform_linux)

# Add a bunch of warnings
target_compile_options(
    replicated-splinterdb PRIVATE
    -Wall -Wpedantic -Wextra -Wshadow -Wno-unused-parameter
    -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls 
    -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Winline
    -Wuninitialized -Wconversion -Wstrict-prototypes -felide-constructors
    -Wno-gcc-compat
)

# Users of our library will need this directory.
target_include_directories(replicated-splinterdb PUBLIC "${ReplicatedSplinterDB_SOURCE_DIR}/include/")

target_link_libraries(replicated-splinterdb nuraft.a rpc splinterdb pthread)

