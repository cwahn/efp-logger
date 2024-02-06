


if(NOT mth_FOUND)


    FetchContent_Declare(mth
    GIT_REPOSITORY "git@gitlab.arsvivendi.io:ars-vivendi/mth.git"
    GIT_TAG main
    )
    
    FetchContent_MakeAvailable(mth)
    set(mth_FOUND TRUE PARENT_SCOPE)
endif()