# Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
# SPDX-License-Identifier: Zlib

# builds examples

macro(_foonathan_add_example name)
    add_executable(foonathan_memory_example_${name} ${name}.cpp)
    target_link_libraries(foonathan_memory_example_${name} PRIVATE foonathan_memory)
endmacro()

_foonathan_add_example(allocator_storage)
_foonathan_add_example(joint_allocation)
_foonathan_add_example(taking_allocators)

if(NOT FOONATHAN_MEMORY_NO_NODE_SIZE)
_foonathan_add_example(tracking)
_foonathan_add_example(using_allocators)
endif()
