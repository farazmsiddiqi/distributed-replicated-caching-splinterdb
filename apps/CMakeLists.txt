add_executable(splinter splinterdb.cpp)
target_compile_features(splinter PRIVATE cxx_std_20)
target_link_libraries(splinter splinterdb)
target_compile_definitions(splinter PRIVATE -DSPLINTERDB_PLATFORM_DIR=platform_linux)

add_executable(raft raft.cxx)
target_compile_features(raft PRIVATE cxx_std_20)
target_link_libraries(raft replicated-splinterdb)
target_link_libraries(raft pthread)