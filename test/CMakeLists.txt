# Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds test

add_executable(foonathan_memory_profiling benchmark.hpp profiling.cpp)
target_link_libraries(foonathan_memory_profiling foonathan_memory)
target_include_directories(foonathan_memory_profiling PRIVATE
                            ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)

# Fetch doctest.
message(STATUS "Fetching doctest")
include(FetchContent)
FetchContent_Declare(doctest URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.9.zip)
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

