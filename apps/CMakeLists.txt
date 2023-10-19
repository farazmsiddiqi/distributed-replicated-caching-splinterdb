include(FetchContent)
find_package(gflags REQUIRED)
FetchContent_GetProperties(gflags)
include_directories(${gflags_INCLUDE_DIR})

add_executable(splinter splinterdb.cpp)
target_compile_features(splinter PRIVATE cxx_std_20)
target_link_libraries(splinter splinterdb)
target_compile_definitions(splinter PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

add_executable(cli cli.cpp)
target_compile_features(cli PRIVATE cxx_std_20)
target_link_libraries(cli replicated-splinterdb)
target_compile_definitions(cli PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

add_executable(server server.cpp)
target_compile_features(server PRIVATE cxx_std_20)
target_link_libraries(server replicated-splinterdb gflags)
target_compile_definitions(server PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

add_executable(client client.cpp)
target_compile_features(client PRIVATE cxx_std_20)
target_link_libraries(client replicated-splinterdb gflags)