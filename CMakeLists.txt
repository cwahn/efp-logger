cmake_minimum_required(VERSION 3.0.0)
project(efp_logger VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(CTest)
enable_testing()

add_library(efp_logger INTERFACE)
target_include_directories(efp_logger INTERFACE include)
target_link_libraries(efp_logger INTERFACE efp fmt)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_subdirectory(lib)
add_subdirectory(test)