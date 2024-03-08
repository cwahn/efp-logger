cmake_minimum_required(VERSION 3.5.0)
project(efp_logger VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(cmake/CPM.cmake)

CPMAddPackage("gh:cwahn/efp#v0.1.0-rc.3")
CPMAddPackage("gh:fmtlib/fmt#10.2.1")

add_library(efp_logger INTERFACE)
target_include_directories(efp_logger INTERFACE include)
target_link_libraries(efp_logger INTERFACE efp fmt::fmt)

# Only if the project is the top-most project
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
    enable_testing()

    add_subdirectory(example)
endif()