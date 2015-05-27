# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds test

file(DOWNLOAD
    https://raw.githubusercontent.com/philsquared/Catch/develop/single_include/catch.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/catch.hpp)

set(tests
        test_allocator.hpp
        main.cpp
        detail/align.cpp
        detail/block_list.cpp
        detail/free_list.cpp
        detail/memory_stack.cpp
        aligned_allocator.cpp
        pool_allocator.cpp
        stack_allocator.cpp)

add_executable(foonathan_memory_test ${tests})
target_link_libraries(foonathan_memory_test foonathan_memory)
target_include_directories(foonathan_memory_test PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR}
                            ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)

enable_testing()
add_test(NAME test COMMAND foonathan_memory_test)
