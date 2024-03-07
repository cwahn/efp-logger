cmake_minimum_required(VERSION 3.5.0)
project(efp_logger VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_library(efp_logger INTERFACE)
target_include_directories(efp_logger INTERFACE include)
target_link_libraries(efp_logger INTERFACE efp fmt::fmt)

add_subdirectory(extern)

# Only if the project is the top-most project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(example)
endif()