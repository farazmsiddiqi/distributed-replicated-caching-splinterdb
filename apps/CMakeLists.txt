include(FetchContent)
find_package(gflags REQUIRED)
FetchContent_GetProperties(gflags)
include_directories(${gflags_INCLUDE_DIR})

add_executable(splinter splinterdb.cpp)
target_compile_features(splinter PRIVATE cxx_std_20)
target_link_libraries(splinter splinterdb)
target_compile_definitions(splinter PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

add_executable(cli cli.cpp)
target_link_libraries(cli replicated-splinterdb)
set_target_properties(cli PROPERTIES LINK_FLAGS_RELEASE -s)

add_executable(server server.cpp)
target_link_libraries(server replicated-splinterdb gflags)
set_target_properties(server PROPERTIES LINK_FLAGS_RELEASE -s)

add_executable(client client.cpp)
target_link_libraries(client replicated-splinterdb gflags)
set_target_properties(client PROPERTIES LINK_FLAGS_RELEASE -s)