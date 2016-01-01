# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds test

add_executable(foonathan_memory_profiling profiling.cpp)
target_link_libraries(foonathan_memory_profiling foonathan_memory)
target_include_directories(foonathan_memory_profiling PRIVATE
                            ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)
comp_target_features(foonathan_memory_profiling PUBLIC CPP11)


file(DOWNLOAD
    https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/catch.downloaded
    SHOW_PROGESS
    STATUS status
    LOG log)

list(GET status 0 status_code)
list(GET status 1 status_string)
if((NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp) AND (NOT status_code EQUAL 0))
    message(FATAL_ERROR "error downloading catch: ${status_string}"
                        "${log}")
else()
    # succesful download, replace old catch.hpp
    file(RENAME ${CMAKE_CURRENT_BINARY_DIR}/catch.downloaded ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)
endif()

set(tests
        test_allocator.hpp
        test.cpp
        detail/align.cpp
        detail/free_list.cpp
        detail/free_list_array.cpp
        detail/memory_stack.cpp
        aligned_allocator.cpp
        allocator_traits.cpp
        default_allocator.cpp
        memory_arena.cpp
        memory_pool.cpp
        memory_pool_collection.cpp
        memory_resource_adapter.cpp
        memory_stack.cpp)

add_executable(foonathan_memory_test ${tests})
target_link_libraries(foonathan_memory_test foonathan_memory)
target_include_directories(foonathan_memory_test PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR}
                            ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)
comp_target_features(foonathan_memory_test PUBLIC CPP11)

enable_testing()
add_test(NAME test COMMAND foonathan_memory_test)
