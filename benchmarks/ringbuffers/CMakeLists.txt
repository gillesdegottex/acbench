# Copyright (C) 2024 Gilles Degottex - All Rights Reserved
#
# You may use, distribute and modify this code under the
# terms of the Apache 2.0 license. You should have
# received a copy of this license with this file.
# If not, please visit:
#     https://github.com/gillesdegottex/acbench

project(benchmark_ringbuffers)

message(STATUS "Benchmarks")

if(ACBENCH_BENCHMARKS_COVERAGE)
    message(WARNING "Benchmarks coverage is enabled (ACBENCH_BENCHMARKS_COVERAGE=ON).\nDo not use this setup for benchmarking as it is sub-optimal for measuring speed efficiency.")
    if(CMAKE_COMPILER_IS_GNUCXX)

        set(GCVOR_COMMAND_BC gcovr -r .. --exclude-throw-branches --exclude-lines-by-pattern '.*GCOVR_EXCL_LINE.*|.*assert.*' --exclude-unreachable-branches -f ../../../acbench/ringbuffer.h)
        add_custom_target(benchmark_ringbuffers_coverage
            DEPENDS benchmark_ringbuffers
            COMMAND ./benchmark_ringbuffers -c 8 -i 10
            COMMAND ${GCVOR_COMMAND_BC} --html-details benchmark_ringbuffers_coverage.html
            COMMAND ${GCVOR_COMMAND_BC} --json benchmark_ringbuffers_coverage.json
            COMMAND ${GCVOR_COMMAND_BC} --json-summary benchmark_ringbuffers_coverage.summary.json
            COMMAND ${GCVOR_COMMAND_BC}
            )
    else()
        message(STATUS "  Unknown compiler. Currently, only GNUCXX is  supported for benchmarks coverage.")
    endif()
else()
    message(STATUS "  Benchmarks coverage is disabled. (ACBENCH_BENCHMARKS_COVERAGE=OFF)")
endif()

acbench_print_expected_compilation_flags()

add_executable(benchmark_ringbuffers main.cpp)

target_include_directories(benchmark_ringbuffers PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_include_directories(benchmark_ringbuffers PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ext/portaudio/src/common/")
target_sources(benchmark_ringbuffers PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../ext/portaudio/src/common/pa_ringbuffer.c")
target_link_libraries(benchmark_ringbuffers PRIVATE jack)
