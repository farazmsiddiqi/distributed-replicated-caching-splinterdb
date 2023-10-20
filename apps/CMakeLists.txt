include(FetchContent)
find_package(gflags REQUIRED)
FetchContent_GetProperties(gflags)
include_directories(${gflags_INCLUDE_DIR})

add_executable(splinter splinterdb.cpp)
target_compile_features(splinter PRIVATE cxx_std_20)
target_link_libraries(splinter splinterdb)
target_compile_definitions(splinter PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

add_executable(spl-server spl_server.cpp)
target_link_libraries(spl-server replicated-splinterdb-server gflags)
set_target_properties(spl-server PROPERTIES LINK_FLAGS_RELEASE -s)

add_executable(spl-client spl_client.cpp)
target_link_libraries(spl-client replicated-splinterdb-client gflags)
set_target_properties(spl-client PROPERTIES LINK_FLAGS_RELEASE -s)