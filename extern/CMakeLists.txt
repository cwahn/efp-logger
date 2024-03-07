# Get the path of the current directory
include(FetchContent)
set(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory("./efp")
add_subdirectory("./fmt")
