# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# root CMakeLists.txt, specifies option and interface library

cmake_minimum_required(VERSION 3.1)
project(FOONATHAN_MEMORY)

# compatibility options
include(cmake/compatibility.cmake)

# debug options
if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(debug_asserts ON)
    set(debug_checks ON)
    set(debug_fence 8)
elseif(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    set(debug_asserts OFF)
    set(debug_checks ON)
    set(debug_fence 0)
else()
    set(debug_asserts OFF)
    set(debug_checks OFF)
    set(debug_fence 0)
endif()

option(FOONATHAN_MEMORY_DEBUG_ASSERT
      "whether or not internal assertions (like the macro assert) are enabled" ${debug_asserts})
option(FOONATHAN_MEMORY_DEBUG_FILL
       "whether or not the (de-)allocated memory will be pre-filled" ${debug_checks})
set(FOONATHAN_MEMORY_DEBUG_FENCE ${debug_fence} CACHE STRING
    "the amount of memory used as fence to help catching overflow errors" )
option(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK
       "whether or not leak checking is active" ${debug_checks})
option(FOONATHAN_MEMORY_DEBUG_POINTER_CHECK
       "whether or not pointer checking on deallocation is active" ${debug_checks})
option(FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK
       "whether or not the (sometimes expensive) check for double deallocation is active" ${debug_asserts})

# other options
set(FOONATHAN_MEMORY_DEFAULT_ALLOCATOR heap_allocator CACHE STRING
    "the default implementation allocator for higher-level ones")
option(FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE
    "whether or not allocator_reference is thread safe by default" ON)
set(FOONATHAN_MEMORY_MEMORY_RESOURCE_HEADER "\"foonathan/pmr.hpp\"" CACHE STRING
    "the header of the memory_resource class used")
set(FOONATHAN_MEMORY_MEMORY_RESOURCE foonathan_comp::memory_resource CACHE STRING
    "the memory_resource class used")
option(FOONATHAN_MEMORY_EXTERN_TEMPLATE
    "whether or not common template instantiations are already provided by the library" ON)

# calculates default values for build_* options
if(COMP_HAS_HOSTED_IMPLEMENTATION AND (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    # examples/tests if toplevel directory (i.e. direct build, not as subdirectory) and hosted
    set(build_examples_tests 1)
else()
    set(build_examples_tests 0)
endif()

if(COMP_HAS_HOSTED_IMPLEMENTATION)
    set(build_tools 1)
else()
    set(build_tools 0)
endif()

option(FOONATHAN_MEMORY_BUILD_EXAMPLES "whether or not to build the examples" ${build_examples_tests})
option(FOONATHAN_MEMORY_BUILD_TESTS "whether or not to build the tests" ${build_examples_tests})
option(FOONATHAN_MEMORY_BUILD_TOOLS "whether or not to build the tools" ${build_tools})

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

set(FOONATHAN_MEMORY_VERSION_MAJOR 0 CACHE STRING "major version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION_MINOR 4 CACHE STRING "minor version of memory" FORCE)

# subdirectories
add_subdirectory(src)
if(FOONATHAN_MEMORY_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
if(FOONATHAN_MEMORY_BUILD_TESTS)
    add_subdirectory(test)
endif()
if(FOONATHAN_MEMORY_BUILD_TOOLS)
    add_subdirectory(tool)
endif()
