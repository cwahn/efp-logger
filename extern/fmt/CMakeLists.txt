if(NOT fmt_FOUND)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(fmt)
endif()