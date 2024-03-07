if(NOT efp_FOUND)

    FetchContent_Declare(efp
    GIT_REPOSITORY "http://github.com/cwahn/efp"
    GIT_TAG v0.1.0-rc.2
    )
    
    FetchContent_MakeAvailable(efp)
    set(efp_FOUND TRUE PARENT_SCOPE)
endif()