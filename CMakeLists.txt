cmake_minimum_required(VERSION 3.5.0)
project(efp_logger VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

include(CTest)
enable_testing()

include(FetchContent)

# efp
# find_package(efp QUIET)

# if(NOT efp_FOUND)
#     FetchContent_Declare(
#         efp
#         GIT_REPOSITORY http://github.com/cwahn/efp.git
#         GIT_TAG dev/trait
#     )
#     FetchContent_MakeAvailable(efp)
# endif()

# # fmt
# find_package(fmt QUIET)

# if(NOT fmt_FOUND)
#     FetchContent_Declare(
#         fmt
#         GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#         GIT_TAG master
#     )
#     FetchContent_MakeAvailable(fmt)
# endif()

# option(ENABLE_ASAN "Enable AddressSanitizer" ON)
# if(ENABLE_ASAN)
#     # Add the -fsanitize=address flag to the compiler and linker
#     add_compile_options(-fsanitize=address)
#     add_link_options(-fsanitize=address)
# endif()

add_library(efp_logger INTERFACE)
target_include_directories(efp_logger INTERFACE include)
target_link_libraries(efp_logger INTERFACE efp fmt::fmt mth)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_subdirectory(extern)

# Check if this CMakeLists.txt is the top-most CMake project
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    add_subdirectory("./example")
endif()