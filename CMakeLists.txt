cmake_minimum_required(VERSION 3.0.0)
project(efp-format VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_library(efp-format INTERFACE)
target_include_directories(efp-format INTERFACE include)
target_link_libraries(efp-format INTERFACE efp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
