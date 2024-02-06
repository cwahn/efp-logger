if(NOT efp_FOUND)

    FetchContent_Declare(efp
    GIT_REPOSITORY "http://github.com/cwahn/efp"
    # GIT_TAG main
    # ! temp
    GIT_TAG dev-enum-rule-of-five
    )
    
    FetchContent_MakeAvailable(efp)
    set(efp_FOUND TRUE PARENT_SCOPE)
endif()