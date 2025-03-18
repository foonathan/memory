# Copyright (C) 2015-2023 Jonathan Müller and foonathan/memory contributors
# SPDX-License-Identifier: Zlib

# root CMakeLists.txt, specifies option and interface library

cmake_minimum_required(VERSION 3.14)
project(FOONATHAN_MEMORY)

set(FOONATHAN_MEMORY_VERSION_MAJOR 0 CACHE STRING "major version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION_MINOR 7 CACHE STRING "minor version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION_PATCH 3 CACHE STRING "patch version of memory" FORCE)
set(FOONATHAN_MEMORY_VERSION "$CACHE{FOONATHAN_MEMORY_VERSION_MAJOR}.$CACHE{FOONATHAN_MEMORY_VERSION_MINOR}.$CACHE{FOONATHAN_MEMORY_VERSION_PATCH}"
                             CACHE STRING "version of memory" FORCE)


# set a debug postfix
set(CMAKE_DEBUG_POSTFIX "-dbg")

# installation destinations
if(UNIX OR VXWORKS)
    include(GNUInstallDirs)

    set(FOONATHAN_MEMORY_INC_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/foonathan_memory")
    set(FOONATHAN_MEMORY_RUNTIME_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}")
    set(FOONATHAN_MEMORY_LIBRARY_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
    set(FOONATHAN_MEMORY_ARCHIVE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")
    set(FOONATHAN_MEMORY_FRAMEWORK_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}")

    set(FOONATHAN_MEMORY_CMAKE_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/foonathan_memory/cmake")
    set(FOONATHAN_MEMORY_ADDITIONAL_FILES_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/foonathan_memory")
elseif(WIN32)
    set(FOONATHAN_MEMORY_INC_INSTALL_DIR "include/foonathan_memory")
    set(FOONATHAN_MEMORY_RUNTIME_INSTALL_DIR   "bin")
    set(FOONATHAN_MEMORY_LIBRARY_INSTALL_DIR   "bin")
    set(FOONATHAN_MEMORY_ARCHIVE_INSTALL_DIR   "lib")
    set(FOONATHAN_MEMORY_FRAMEWORK_INSTALL_DIR "bin")

    set(FOONATHAN_MEMORY_CMAKE_CONFIG_INSTALL_DIR "share/foonathan_memory/cmake")
    set(FOONATHAN_MEMORY_ADDITIONAL_FILES_INSTALL_DIR "share/foonathan_memory")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
else()
    message(FATAL_ERROR "Could not set install folders for this platform!")
endif()

include(cmake/configuration.cmake)
include(cmake/atomic.cmake)
include(cmake/get_container_node_sizes.cmake)

# subdirectories
add_subdirectory(src)
if(FOONATHAN_MEMORY_BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
if(FOONATHAN_MEMORY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
if(FOONATHAN_MEMORY_BUILD_TOOLS)
    add_subdirectory(tool)
endif()

# install readme and license
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_CURRENT_SOURCE_DIR}/README.md" DESTINATION ${FOONATHAN_MEMORY_ADDITIONAL_FILES_INSTALL_DIR})

install(EXPORT foonathan_memoryTargets DESTINATION ${FOONATHAN_MEMORY_CMAKE_CONFIG_INSTALL_DIR}
                                       FILE foonathan_memory-config.cmake)


