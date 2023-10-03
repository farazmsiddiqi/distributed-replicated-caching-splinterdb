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
target_compile_definitions(replicated-splinterdb PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

# Users of our library will need this directory.
target_include_directories(replicated-splinterdb PUBLIC "${ReplicatedSplinterDB_SOURCE_DIR}/include/")

target_link_libraries(replicated-splinterdb nuraft)
target_link_libraries(replicated-splinterdb splinterdb)

# All users of this library will need at least C++20
target_compile_features(replicated-splinterdb PUBLIC cxx_std_20)
target_compile_options(replicated-splinterdb PRIVATE -O2)