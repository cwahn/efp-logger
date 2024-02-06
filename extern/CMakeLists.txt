# Get the path of the current directory
set(CURRENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Find all subdirectories within the current directory
file(GLOB SUBDIRS RELATIVE ${CURRENT_DIR} ${CURRENT_DIR}/*)
foreach(SUBDIR IN LISTS SUBDIRS)
    if(IS_DIRECTORY ${CURRENT_DIR}/${SUBDIR})
        # Check if a CMakeLists.txt file exists in the subdirectory
        if(EXISTS "${CURRENT_DIR}/${SUBDIR}/CMakeLists.txt")
            # Add the subdirectory to the build
            add_subdirectory(${SUBDIR})
        endif()
    endif()
endforeach()