cmake_minimum_required(VERSION 3.5.0)
project(efp_logger VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_library(efp_logger INTERFACE)
target_include_directories(efp_logger INTERFACE include)
target_link_libraries(efp_logger INTERFACE efp fmt)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# dependancies
include(FetchContent)

# efp
find_package(efp QUIET)

if(NOT efp_FOUND)
    FetchContent_Declare(
        efp
        GIT_REPOSITORY http://github.com/cwahn/efp
        GIT_TAG main
    )

    FetchContent_MakeAvailable(efp)
endif()

#fmt
find_package(fmt QUIET)

if(NOT fmt_FOUND)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt
    )
    FetchContent_MakeAvailable(fmt)
endif()

add_subdirectory(test)