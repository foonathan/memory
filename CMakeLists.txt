# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# root CMakeLists.txt, specifies option and interface library

cmake_minimum_required(VERSION 3.1)
project(FOONATHAN_MEMORY)

set(FOONATHAN_MEMORY_VERSION_MAJOR 0 CACHE STRING "major version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION_MINOR 6 CACHE STRING "minor version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION_PATCH 1 CACHE STRING "patch version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION "${FOONATHAN_MEMORY_VERSION_MAJOR}.${FOONATHAN_MEMORY_VERSION_MINOR}.${FOONATHAN_MEMORY_VERSION_PATCH}"
                             CACHE STRING "version of memory" FORCE)

include(cmake/compatibility.cmake)
include(cmake/configuration.cmake)

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

# install readme and license
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION ${FOONATHAN_MEMORY_ADDITIONAL_FILES_INSTALL_DIR})

install(EXPORT foonathan_memoryTargets DESTINATION ${FOONATHAN_MEMORY_CMAKE_CONFIG_INSTALL_DIR}
		                               FILE foonathan_memory-config.cmake)


