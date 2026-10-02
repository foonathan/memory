# Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
# SPDX-License-Identifier: Zlib

# builds test

add_executable(foonathan_memory_profiling benchmark.hpp profiling.cpp)
target_link_libraries(foonathan_memory_profiling foonathan_memory)
target_include_directories(foonathan_memory_profiling PRIVATE
                            ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)

# Fetch doctest.
message(STATUS "Fetching doctest")
include(FetchContent)
FetchContent_Declare(doctest URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.12.zip)
FetchContent_MakeAvailable(doctest)

set(tests
    test_allocator.hpp
    test.cpp
    detail/align.cpp
    detail/debug_helpers.cpp
    detail/free_list.cpp
    detail/free_list_array.cpp
    detail/ilog2.cpp
    detail/memory_stack.cpp
    aligned_allocator.cpp
    allocator_traits.cpp
    default_allocator.cpp
    fallback_allocator.cpp
    iteration_allocator.cpp
    joint_allocator.cpp
    memory_arena.cpp
    memory_pool.cpp
    memory_pool_collection.cpp
    memory_resource_adapter.cpp
    memory_stack.cpp
    segregator.cpp
    smart_ptr.cpp)

add_executable(foonathan_memory_test ${tests})
target_link_libraries(foonathan_memory_test PRIVATE foonathan_memory doctest::doctest)
target_include_directories(foonathan_memory_test PRIVATE
                            ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)

add_test(NAME test COMMAND foonathan_memory_test)

# Compile each implementation separately so inline comparisons cannot be resolved
# to the hosted implementation in foonathan_memory.
foreach(hosted 0 1)
    set(target foonathan_memory_pointer_comparisons_${hosted})
    add_executable(${target} test.cpp detail/free_list_utils.cpp)
    target_link_libraries(${target} PRIVATE doctest::doctest)
    target_include_directories(${target} PRIVATE
        ${FOONATHAN_MEMORY_SOURCE_DIR}/src
        ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory
        ${FOONATHAN_MEMORY_BINARY_DIR}/src)
    target_compile_definitions(${target} PRIVATE FOONATHAN_HOSTED_IMPLEMENTATION=${hosted})
    target_compile_features(${target} PRIVATE cxx_std_11)
    add_test(NAME pointer_comparisons_${hosted} COMMAND ${target})
endforeach()
