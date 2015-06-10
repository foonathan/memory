# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# root CMakeLists.txt, specifies option and interface library

if (BIICODE)
    include(biicode/cmake/tools)

    ADD_BII_TARGETS()
    ACTIVATE_CPP11(INTERFACE ${BII_BLOCK_TARGET})
else()
    cmake_minimum_required(VERSION 3.1)
    project(FOONATHAN_MEMORY)
endif()

# compatibility options
include(cmake/compatibility.cmake)

# debug options
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(debug_checks ON)
    set(debug_fence 8)
elseif(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    set(debug_checks ON)
    set(debug_fence 0)
else()
    set(debug_checks OFF)
    set(debug_fence 0)
endif()

option(FOONATHAN_MEMORY_DEBUG_FILL
       "whether or not the (de-)allocated memory will be pre-filled" ${debug_checks})
set(FOONATHAN_MEMORY_DEBUG_FENCE ${debug_fence} CACHE STRING
    "the amount of memory used as fence to help catching overflow errors" )
option(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
       "whether or not leak checking is active" ${debug_checks})
option(FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
       "whether or not pointer checking on deallocation is active" ${debug_checks})

# other options
set(FOONATHAN_MEMORY_DEFAULT_ALLOCATOR heap_allocator CACHE STRING
    "the default implementation allocator for higher-level ones")
option(FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE
    "whether or not allocator_reference is thread safe by default" ON)

# whether or not this is project is build directly or as subdirectory
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(is_toplevel 1)
else()
    set(is_toplevel 0)
endif()

# build options
option(FOONATHAN_MEMORY_BUILD_EXAMPLES "whether or not to build the examples" ${is_toplevel})
option(FOONATHAN_MEMORY_BUILD_TESTS "whether or not to build the tests" ${is_toplevel})
option(FOONATHAN_MEMORY_INCLUDE_PREFIX "whether or not you have to use <foonathan/memory/xxx.hpp>" OFF)
option(FOONATHAN_MEMORY_NAMESPACE_PREFIX "whether or not everything is in namespace foonathan::memory" OFF)

# variables to use library in other projects
if(FOONATHAN_MEMORY_INCLUDE_PREFIX)
    set(FOONATHAN_MEMORY_INCLUDE_DIR ${FOONATHAN_MEMORY_SOURCE_DIR}/include/ CACHE PATH
         "include directory for library")
else()
    set(FOONATHAN_MEMORY_INCLUDE_DIR ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan CACHE PATH
          "include directory for library")
endif()

set(FOONATHAN_MEMORY_VERSION_MAJOR 0 CACHE STRING "major version")
set(FOONATHAN_MEMORY_VERSION_MINOR 3 CACHE STRING "minor version")

# subdirectories
add_subdirectory(src)
if(FOONATHAN_MEMORY_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
if(FOONATHAN_MEMORY_BUILD_TESTS)
    add_subdirectory(test)
endif()
