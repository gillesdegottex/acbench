# Copyright (C) 2024 Gilles Degottex - All Rights Reserved
#
# You may use, distribute and modify this code under the
# terms of the Apache 2.0 license. You should have
# received a copy of this license with this file.
# If not, please visit:
#     https://github.com/gillesdegottex/acbench

cmake_minimum_required(VERSION 3.22)

project(acbench)

# Compilation options ---------------------------------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # To generate compile_commands.json to allow clangd do the code navigation
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# By default, all options are building the smallest and faster library
option(ACBENCH_ASSERT "If ON, standard C asserts will be enabled by removing -DNDEBUG compilation flag (no matter the build type, release or not)." OFF)
option(ACBENCH_TESTS "Build the tests" OFF)
option(ACBENCH_TESTS_COVERAGE "Add compilation options for testing coverage." OFF)
option(ACBENCH_BENCHMARKS "Build the benchmarks (will need all implementations)" OFF)
option(ACBENCH_BENCHMARKS_COVERAGE "Add compilation options for testing coverage during benchmarking (WARNING: Doing so nullify the value of the speed measurements as the coverage measure slows down the speed efficiency. Accordingly this must NOT be used when benchmarking the methods speed.)." OFF)

# Optimisation ----------------------------------------------------------------
message(STATUS "")

if(ACBENCH_TESTS_COVERAGE OR ACBENCH_BENCHMARKS_COVERAGE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  message(WARNING "The build type is in debug (CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}), which is sub-optimal for measuring speed efficiency.\nSome options activated this build option, see below.")

else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if (WIN32)
  # TODO 
else()
  # -O3 is automatically added
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Ofast")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused-parameter")
  # set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # TODO(GD) Check it too
  # TODO(GD) Read https://simonbyrne.github.io/notes/fastmath/
  # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()

# -----------------------------------------------------------------------------

# Keep control of asserts behavior independently of CMAKE_BUILD_TYPE
if(ACBENCH_ASSERT)
  message(WARNING "Removed -DNDEBUG from compilation flags in order to enable C asserts. This should not be used when benchmarking as it will add assert calls that will slow down the executions (ACBENCH_ASSERT=ON)")
  string(REGEX REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
  string(REGEX REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
  string(REGEX REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
  string(REGEX REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
  message(STATUS "C asserts are disabled. (ACBENCH_ASSERT=OFF)")
endif()

function(acbench_print_expected_compilation_flags)
  set(CMAKE_C_FLAGS_FINAL "${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS_FINAL "${CMAKE_CXX_FLAGS}")
  if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(CMAKE_C_FLAGS_FINAL "${CMAKE_C_FLAGS_FINAL} ${CMAKE_C_FLAGS_RELEASE}")
    set(CMAKE_CXX_FLAGS_FINAL "${CMAKE_CXX_FLAGS_FINAL} ${CMAKE_CXX_FLAGS_RELEASE}")
  elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(CMAKE_C_FLAGS_FINAL "${CMAKE_C_FLAGS_FINAL} ${CMAKE_C_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_FINAL "${CMAKE_CXX_FLAGS_FINAL} ${CMAKE_CXX_FLAGS_DEBUG}")
  else()
    message(WARNING "Unknown build type CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  endif()

  message(STATUS "  Expected compilation flags, as provided to the compiler")
  message(STATUS "    C_FLAGS=${CMAKE_C_FLAGS_FINAL}")
  message(STATUS "    CXX_FLAGS=${CMAKE_CXX_FLAGS_FINAL}")
endfunction()

if(ACBENCH_TESTS_COVERAGE OR ACBENCH_BENCHMARKS_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags()
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-inline -fno-inline-small-functions -fno-default-inline")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline -fno-inline-small-functions -fno-default-inline")
endif()

if(ACBENCH_TESTS)
  message(STATUS "Tests")

  include(CTest)

  message(STATUS "  Fetching Catch2...")
  # TODO(GD) Use the one in ext/
  # find_package(Catch2 2 REQUIRED)
  Include(FetchContent)
    FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.3.0 # or a later release
  )
  FetchContent_MakeAvailable(Catch2)

  if(ACBENCH_TESTS_COVERAGE)
    message(WARNING "Test coverage is enabled (ACBENCH_TESTS_COVERAGE=ON).\nDo not use this setup for benchmarking as it is sub-optimal for measuring speed efficiency.")

    if(CMAKE_COMPILER_IS_GNUCXX)

        # set(GCVOR_COMMAND_TESTS gcovr --gcov-executable gcov-5 -r .. --exclude-throw-branches --exclude-lines-by-pattern '.*GCOVR_EXCL_LINE.*|.*assert.*' --exclude-unreachable-branches -f ../acbench/ringbuffer.h)
        set(GCVOR_COMMAND_TESTS gcovr -r .. --gcov-ignore-parse-errors --exclude-throw-branches --exclude-lines-by-pattern '.*GCOVR_EXCL_LINE.*|.*assert.*' --exclude-unreachable-branches -f ../acbench/ringbuffer.h)
        add_custom_target(test_coverage
            DEPENDS ringbuffer_test
            COMMAND ./ringbuffer_test
            COMMAND cmake --version
            COMMAND python3 --version
            COMMAND gcc --version
            COMMAND gcovr --version
            COMMAND ${GCVOR_COMMAND_TESTS} --html-details ringbuffer_test_coverage.html
            COMMAND ${GCVOR_COMMAND_TESTS} --json ringbuffer_test_coverage.json
            COMMAND ${GCVOR_COMMAND_TESTS} --json-summary ringbuffer_test_coverage.summary.json
            COMMAND ${GCVOR_COMMAND_TESTS}
            COMMAND python3 ../coverage_test.py
          )
    else()
      message(STATUS "  Unknown compiler. Currently, only GNUCXX is  supported for testing coverage.")
    endif()
  else()
    message(STATUS "  Test coverage is disabled. (ACBENCH_TESTS_COVERAGE=OFF)")
  endif()

  acbench_print_expected_compilation_flags()

  add_executable(ringbuffer_test acbench/ringbuffer_test.cpp)
  target_include_directories(ringbuffer_test PUBLIC ${PROJECT_SOURCE_DIR})
  target_link_libraries(ringbuffer_test PRIVATE Catch2::Catch2WithMain)
  add_test(NAME ringbuffer_test COMMAND ringbuffer_test)
endif()

if(ACBENCH_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
