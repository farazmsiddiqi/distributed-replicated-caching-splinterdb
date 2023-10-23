file(GLOB SERVER_HEADER_LIST CONFIGURE_DEPENDS
    "${ReplicatedSplinterDB_SOURCE_DIR}/include/server/*.h"
    "${ReplicatedSplinterDB_SOURCE_DIR}/include/common/*.h")

file(GLOB SERVER_SOURCE_LIST CONFIGURE_DEPENDS
    "${ReplicatedSplinterDB_SOURCE_DIR}/src/server/*.cpp"
    "${ReplicatedSplinterDB_SOURCE_DIR}/src/common/*.cpp")

# Make the static libraries that are to be linked to the main executables
add_library(replicated-splinterdb-server STATIC ${SERVER_SOURCE_LIST} ${SERVER_HEADER_LIST})

# All users of these libraries will need at least C++20
target_compile_features(replicated-splinterdb-server PUBLIC cxx_std_20)

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

# Users of our libraries will need these include directories
target_include_directories(replicated-splinterdb-server PUBLIC "${ReplicatedSplinterDB_SOURCE_DIR}/include/server/")

# Link the libraries to some other dependencies
target_link_libraries(replicated-splinterdb-server nuraft.a rpc splinterdb pthread)
