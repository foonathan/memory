# Copyright (C) 2015-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds actual library

set(header_path ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)
set(detail_header
        ${header_path}/detail/align.hpp
        ${header_path}/detail/assert.hpp
        ${header_path}/detail/container_node_sizes.hpp
        ${header_path}/detail/debug_helpers.hpp
        ${header_path}/detail/ebo_storage.hpp
        ${header_path}/detail/free_list.hpp
        ${header_path}/detail/free_list_array.hpp
        ${header_path}/detail/ilog2.hpp
        ${header_path}/detail/lowlevel_allocator.hpp
        ${header_path}/detail/memory_stack.hpp
        ${header_path}/detail/small_free_list.hpp
        ${header_path}/detail/utility.hpp)
set(header
        ${header_path}/aligned_allocator.hpp
        ${header_path}/allocator_storage.hpp
        ${header_path}/allocator_traits.hpp
        ${header_path}/config.hpp
        ${header_path}/container.hpp
        ${header_path}/debugging.hpp
        ${header_path}/default_allocator.hpp
        ${header_path}/deleter.hpp
        ${header_path}/error.hpp
        ${header_path}/fallback_allocator.hpp
        ${header_path}/malloc_allocator.hpp
        ${header_path}/heap_allocator.hpp
        ${header_path}/iteration_allocator.hpp
        ${header_path}/joint_allocator.hpp
        ${header_path}/memory_arena.hpp
        ${header_path}/memory_pool.hpp
        ${header_path}/memory_pool_collection.hpp
        ${header_path}/memory_pool_type.hpp
        ${header_path}/memory_resource_adapter.hpp
        ${header_path}/memory_stack.hpp
        ${header_path}/namespace_alias.hpp
        ${header_path}/new_allocator.hpp
        ${header_path}/segregator.hpp
        ${header_path}/smart_ptr.hpp
        ${header_path}/static_allocator.hpp
        ${header_path}/std_allocator.hpp
        ${header_path}/temporary_allocator.hpp
        ${header_path}/threading.hpp
        ${header_path}/tracking.hpp
        ${header_path}/virtual_memory.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp)

set(src
        detail/align.cpp
        detail/debug_helpers.cpp
        detail/assert.cpp
        detail/free_list.cpp
        detail/free_list_array.cpp
        detail/free_list_utils.hpp
        detail/small_free_list.cpp
        debugging.cpp
        error.cpp
        heap_allocator.cpp
        iteration_allocator.cpp
        malloc_allocator.cpp
        memory_arena.cpp
        memory_pool.cpp
        memory_pool_collection.cpp
        memory_stack.cpp
        new_allocator.cpp
        static_allocator.cpp
        temporary_allocator.cpp
        virtual_memory.cpp)

# configure config file
configure_file("config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config_impl.hpp")

# generate container_node_sizes.hpp
# don't run it while cross-compiling and CMAKE_CROSSCOMPILING_EMULATOR is not defined
if(FOONATHAN_MEMORY_BUILD_TOOLS)
  if(NOT CMAKE_CROSSCOMPILING)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
            COMMAND $<TARGET_FILE:foonathan_memory_node_size_debugger> --code ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
            DEPENDS foonathan_memory_node_size_debugger
            VERBATIM)
  elseif(DEFINED CMAKE_CROSSCOMPILING_EMULATOR)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
            COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:foonathan_memory_node_size_debugger> --code ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
            DEPENDS foonathan_memory_node_size_debugger
            VERBATIM)
  elseif(QNX)
    # currently this process was only tested on Linux and Windows. Linux matched the QNX generated header file but Windows did not
    # Mac still needs to be tested. If passed the test then could be added here as well.
    if( CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7")
      execute_process(COMMAND g++ -DVERSION="${FOONATHAN_MEMORY_VERSION_MAJOR}.${FOONATHAN_MEMORY_VERSION_MINOR}.${FOONATHAN_MEMORY_VERSION_PATCH}" -o ${PROJECT_SOURCE_DIR}/tool/nodesize_dbg ${PROJECT_SOURCE_DIR}/tool/node_size_debugger.cpp )
      add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
              COMMAND ${PROJECT_SOURCE_DIR}/tool/nodesize_dbg --code ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp)
    elseif(EXISTS "${PROJECT_SOURCE_DIR}/tool/container_node_sizes_impl.hpp")
      message("-- Using manually generated file: tool/container_node_sizes_impl.hpp")
      add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
              COMMAND cp ${PROJECT_SOURCE_DIR}/tool/container_node_sizes_impl.hpp ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp )
    else()
      message(FATAL_ERROR "\nCannot generate container_node_sizes_impl.hpp on ${CMAKE_HOST_SYSTEM_NAME}.\n"
                            "Please manually generate the header file container_node_sizes_impl.hpp by doing the following:\n"
                            "- Build nodesize_dbg from source:\n"
                            "	${PROJECT_SOURCE_DIR}/tool/node_size_debugger.cpp \n"
                            "- Transfer file to QNX target and execute:\n"
                            "	nodesize_dbg --code > container_node_sizes_impl.hpp \n"
                            "- Transfer generated header file back to: \n"
                            "	${PROJECT_SOURCE_DIR}/tool and REBUILD")
    endif()
  else()
    message(WARNING "cross-compiling, but emulator is not defined, "
                    "cannot generate container_node_sizes_impl.hpp, node size information will be unavailable")
    file(WRITE  ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp "#define FOONATHAN_MEMORY_NO_NODE_SIZE")
    set(FOONATHAN_MEMORY_NO_NODE_SIZE 1 PARENT_SCOPE)
  endif()
else()
    message(WARNING "cannot generate container_node_sizes_impl.hpp, node size information will be unavailable")
    file(WRITE  ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp "#define FOONATHAN_MEMORY_NO_NODE_SIZE")
    set(FOONATHAN_MEMORY_NO_NODE_SIZE 1 PARENT_SCOPE)
endif()

add_library(foonathan_memory ${detail_header} ${header} ${src})
target_include_directories(foonathan_memory PUBLIC $<BUILD_INTERFACE:${FOONATHAN_MEMORY_SOURCE_DIR}/include/> # for client in subdirectory
                                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # for generated files in build mode
                                                   $<INSTALL_INTERFACE:${FOONATHAN_MEMORY_INC_INSTALL_DIR}> # for client in install mode
                                            PRIVATE ${header_path}) # for source files
target_compile_definitions(foonathan_memory PUBLIC
                            FOONATHAN_MEMORY=1
                            FOONATHAN_MEMORY_VERSION_MAJOR=${FOONATHAN_MEMORY_VERSION_MAJOR}
                            FOONATHAN_MEMORY_VERSION_MINOR=${FOONATHAN_MEMORY_VERSION_MINOR}
                            FOONATHAN_MEMORY_VERSION_PATCH=${FOONATHAN_MEMORY_VERSION_PATCH})
if(NOT MSVC)
target_compile_features(foonathan_memory PUBLIC cxx_constexpr)
endif()

set_target_properties(foonathan_memory PROPERTIES
                                       OUTPUT_NAME "foonathan_memory-${FOONATHAN_MEMORY_VERSION}"
                                       POSITION_INDEPENDENT_CODE ON)

install(TARGETS foonathan_memory EXPORT foonathan_memoryTargets
        RUNTIME       DESTINATION ${FOONATHAN_MEMORY_RUNTIME_INSTALL_DIR}
        LIBRARY       DESTINATION ${FOONATHAN_MEMORY_LIBRARY_INSTALL_DIR}
        ARCHIVE       DESTINATION ${FOONATHAN_MEMORY_ARCHIVE_INSTALL_DIR}
        FRAMEWORK     DESTINATION ${FOONATHAN_MEMORY_FRAMEWORK_INSTALL_DIR})

# Write/install version file
include(CMakePackageConfigHelpers)
set(version_file "${CMAKE_CURRENT_BINARY_DIR}/cmake/foonathan_memory-config-version.cmake")
write_basic_package_version_file(${version_file}
                                 VERSION ${FOONATHAN_MEMORY_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config_impl.hpp DESTINATION ${FOONATHAN_MEMORY_INC_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp DESTINATION ${FOONATHAN_MEMORY_INC_INSTALL_DIR}/foonathan/memory/detail)
install(FILES ${header}                                   DESTINATION ${FOONATHAN_MEMORY_INC_INSTALL_DIR}/foonathan/memory)
install(FILES ${detail_header}                            DESTINATION ${FOONATHAN_MEMORY_INC_INSTALL_DIR}/foonathan/memory/detail)
install(FILES ${version_file}                             DESTINATION ${FOONATHAN_MEMORY_CMAKE_CONFIG_INSTALL_DIR})
