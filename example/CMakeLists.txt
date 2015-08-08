# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds examples

if(BIICODE)
    return()
endif()

macro(_foonathan_add_example name)
    add_executable(foonathan_memory_example_${name} ${name}.cpp)
    target_link_libraries(foonathan_memory_example_${name} PRIVATE foonathan_memory)
    target_include_directories(foonathan_memory_example_${name} PRIVATE
                               ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/)
    comp_target_features(foonathan_memory_example_${name} PUBLIC CPP11)
endmacro()

_foonathan_add_example(allocator)
_foonathan_add_example(smart_ptr)
_foonathan_add_example(temporary)
