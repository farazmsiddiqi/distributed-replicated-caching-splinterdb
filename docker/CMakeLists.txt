# Import library shared objects and include directories
link_directories("${CMAKE_SOURCE_DIR}/libs")
include_directories("${CMAKE_SOURCE_DIR}/nuraft/include/")
include_directories("${CMAKE_SOURCE_DIR}/splinterdb/include/")

# Include executable subdirectories
add_subdirectory(apps)