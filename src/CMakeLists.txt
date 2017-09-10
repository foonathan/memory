# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds actual library

set(header_path ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)
set(detail_header
        ${header_path}/detail/align.hpp
        ${header_path}/detail/assert.hpp
        ${header_path}/detail/container_node_sizes.hpp
        ${header_path}/detail/debug_helpers.hpp
        ${header_path}/detail/free_list.hpp
        ${header_path}/detail/free_list_array.hpp
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
        detail/ilog2.hpp
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
if(FOONATHAN_MEMORY_BUILD_TOOLS AND (NOT CMAKE_CROSS_COMPILING))
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
            COMMAND foonathan_memory_node_size_debugger --code --alignof "FOONATHAN_ALIGNOF(T)" ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp
            DEPENDS foonathan_memory_node_size_debugger
            VERBATIM)
else()
    message(WARNING "cannot generate container_node_sizes_impl.hpp, node size information will be unavailable")
    file(WRITE  ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes_impl.hpp "")
endif()

add_library(foonathan_memory ${detail_header} ${header} ${src})
_foonathan_use_comp(foonathan_memory)
comp_target_features(foonathan_memory PRIVATE CPP11)
target_include_directories(foonathan_memory PUBLIC $<BUILD_INTERFACE:${FOONATHAN_MEMORY_SOURCE_DIR}/include/> # for client in subdirectory
                                                   $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # for generated files in build mode
                                                   $<INSTALL_INTERFACE:${include_dest}> # for client in install mode
                                                   $<INSTALL_INTERFACE:${lib_dest}> # for generated files in install mdoe
                                            PRIVATE ${header_path}) # for source files
target_compile_definitions(foonathan_memory PUBLIC
                            FOONATHAN_MEMORY=1
                            FOONATHAN_MEMORY_VERSION_MAJOR=${FOONATHAN_MEMORY_VERSION_MAJOR}
                            FOONATHAN_MEMORY_VERSION_MINOR=${FOONATHAN_MEMORY_VERSION_MINOR})

# installation
install(TARGETS foonathan_memory EXPORT foonathan_memory  DESTINATION ${lib_dest})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config_impl.hpp DESTINATION ${lib_dest})
install(FILES ${header}                                   DESTINATION ${include_dest}/foonathan/memory)
install(FILES ${detail_header}                            DESTINATION ${include_dest}/foonathan/memory/detail)
