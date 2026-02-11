cmake_minimum_required(VERSION 3.18)
project(weasel VERSION 1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB_RECURSE HEADER_FILES "src/*.h")
list(REMOVE_ITEM SRC_FILES "src/main.cpp")

if(MSVC)
    add_compile_options(/utf-8 /W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

add_library(core_lib STATIC ${SRC_FILES} ${HEADER_FILES})
target_include_directories(core_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(start src/main.cpp)
target_link_libraries(start PRIVATE core_lib)

option(BUILD_TESTING "Build the tests" ON)

if (BUILD_TESTING)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_FILES "tests/*.cpp")

    if (TEST_FILES)
        add_executable(tests ${TEST_FILES})
        target_link_libraries(tests PRIVATE core_lib GTest::gmock_main)

        include(GoogleTest)
        gtest_discover_tests(tests)
    endif ()
endif ()

option(BUILD_BENCHMARKS "Build the benchmarks" ON)

if (BUILD_BENCHMARKS)
    include(FetchContent)

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
            googlebenchmark
            URL https://github.com/google/benchmark/archive/refs/tags/v1.8.3.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googlebenchmark)

    file(GLOB_RECURSE BENCHMARK_FILES "benchmarks/*.cpp")

    if (BENCHMARK_FILES)
        add_executable(benchmarks ${BENCHMARK_FILES})
        target_link_libraries(benchmarks PRIVATE core_lib benchmark::benchmark_main)
    endif ()
endif ()