# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# defines configuration options
# note: only include it in memory's top-level CMakeLists.txt, after compatibility.cmake

# what to build
# examples/tests if toplevel directory (i.e. direct build, not as subdirectory) and hosted
# tools if hosted
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(build_examples_tests 1)
else()
    set(build_examples_test 0)
endif()
set(build_tools 1)

option(FOONATHAN_MEMORY_BUILD_EXAMPLES "whether or not to build the examples" ${build_examples_tests})
option(FOONATHAN_MEMORY_BUILD_TESTS "whether or not to build the tests" ${build_examples_tests})
option(FOONATHAN_MEMORY_BUILD_TOOLS "whether or not to build the tools" ${build_tools})

# debug options, pre-set by build type
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(FOONATHAN_MEMORY_DEBUG_ASSERT ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_FILL ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_FENCE 8 CACHE STRING "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_POINTER_CHECK ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK ON CACHE BOOL "" FORCE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    set(FOONATHAN_MEMORY_DEBUG_ASSERT OFF CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_FILL ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_FENCE 0 CACHE STRING "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_POINTER_CHECK ON CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK OFF CACHE BOOL "" FORCE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(FOONATHAN_MEMORY_DEBUG_ASSERT OFF CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_FILL OFF CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_FENCE 0 CACHE STRING "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK OFF CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_POINTER_CHECK OFF CACHE BOOL "" FORCE)
    set(FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK OFF CACHE BOOL "" FORCE)
else()
    option(FOONATHAN_MEMORY_DEBUG_ASSERT
            "whether or not internal assertions (like the macro assert) are enabled" OFF)
    option(FOONATHAN_MEMORY_DEBUG_FILL
            "whether or not the (de-)allocated memory will be pre-filled" OFF)
    set(FOONATHAN_MEMORY_DEBUG_FENCE 0 CACHE STRING
            "the amount of memory used as fence to help catching overflow errors" )
    option(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
            "whether or not leak checking is active" OFF)
    option(FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
            "whether or not pointer checking on deallocation is active" OFF)
    option(FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK
            "whether or not the (sometimes expensive) check for double deallocation is active" OFF)
endif()

# other options
option(FOONATHAN_MEMORY_CHECK_ALLOCATION_SIZE
        "whether or not the size of the allocation will be checked" ON)
set(FOONATHAN_MEMORY_DEFAULT_ALLOCATOR heap_allocator CACHE STRING
    "the default implementation allocator for higher-level ones")
set(FOONATHAN_MEMORY_MEMORY_RESOURCE_HEADER "<memory_resource>" CACHE STRING
    "the header of the memory_resource class used")
set(FOONATHAN_MEMORY_MEMORY_RESOURCE std::memory_resource CACHE STRING
    "the memory_resource class used")
option(FOONATHAN_MEMORY_EXTERN_TEMPLATE
    "whether or not common template instantiations are already provided by the library" ON)
set(FOONATHAN_MEMORY_TEMPORARY_STACK_MODE 2 CACHE STRING
     "set to 0 to disable the per-thread stack completely, to 1 to disable the nitfy counter and to 2 to enable everything")
set(FOONATHAN_MEMORY_CONTAINER_NODE_SIZES_IMPL container_node_sizes_impl.hpp CACHE FILEPATH
     "the path of the header that defines the node sizes and alignments if pre-generated.")
