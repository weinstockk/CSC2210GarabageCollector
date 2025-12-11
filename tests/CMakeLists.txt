# Catch2 (header-only)
include(FetchContent)
FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0
)

FetchContent_MakeAvailable(catch2)

add_executable(tests
        test_gc_basic.cpp
)

target_link_libraries(tests
        PRIVATE
        GC
        Catch2::Catch2WithMain
)

add_test(NAME tests COMMAND tests)
