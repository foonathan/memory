# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds actual library

set(header_path ${FOONATHAN_MEMORY_SOURCE_DIR}/include/foonathan/memory)
set(header
        ${header_path}/detail/align.hpp
        ${header_path}/detail/block_list.hpp
        ${header_path}/detail/free_list.hpp
        ${header_path}/detail/free_list_array.hpp
        ${header_path}/detail/memory_stack.hpp
        ${header_path}/detail/small_free_list.hpp
        ${header_path}/detail/utility.hpp
        ${header_path}/aligned_allocator.hpp
        ${header_path}/allocator_adapter.hpp
        ${header_path}/allocator_traits.hpp
        ${header_path}/config.hpp
        ${header_path}/container.hpp
        ${header_path}/debugging.hpp
        ${header_path}/default_allocator.hpp
        ${header_path}/deleter.hpp
        ${header_path}/error.hpp
        ${header_path}/heap_allocator.hpp
        ${header_path}/memory_pool.hpp
        ${header_path}/memory_pool_collection.hpp
        ${header_path}/memory_pool_type.hpp
        ${header_path}/memory_stack.hpp
        ${header_path}/new_allocator.hpp
        ${header_path}/smart_ptr.hpp
        ${header_path}/std_allocator.hpp
        ${header_path}/temporary_allocator.hpp
        ${header_path}/threading.hpp
        ${header_path}/tracking.hpp)

set(src
        detail/block_list.cpp
        detail/free_list.cpp
        detail/free_list_array.cpp
        detail/memory_stack.cpp
        detail/small_free_list.cpp
        debugging.cpp
        error.cpp
        heap_allocator.cpp
        new_allocator.cpp
        temporary_allocator.cpp)

if(BIICODE)
    set(foonathan_memory ${BII_LIB_TARGET})
else()
    add_library(foonathan_memory ${header} ${src})
endif()

target_include_directories(foonathan_memory PRIVATE ${header_path})
target_include_directories(foonathan_memory PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # for config_impl.hpp
target_include_directories(foonathan_memory INTERFACE ${FOONATHAN_MEMORY_INCLUDE_DIR}) # for other targets using it
_foonathan_use_comp(foonathan_memory) # setup compatiblity code

# configure config file
configure_file("config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config_impl.hpp")

# copy container_node_sizes.hpp.in to binary dir without generating code
file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes.hpp")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/container_node_sizes.hpp.in"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(RENAME "${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes.hpp.in"
            "${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes.hpp")

if(FOONATHAN_MEMORY_BUILD_TOOLS AND (NOT CMAKE_CROSS_COMPILING))
    # custom command that generates the node sizes
    # it is technically a hack to ensure that the command will only run once
    # only works because the header container.hpp isn't actually used inside the library
    add_custom_command(TARGET foonathan_memory PRE_BUILD
        COMMAND foonathan_memory_node_size_debugger --code --append --alignof "FOONATHAN_ALIGNOF(T)" ${CMAKE_CURRENT_BINARY_DIR}/container_node_sizes.hpp
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/container_node_sizes.hpp.in"
        DEPENDS foonathan_memory_node_size_debugger
        VERBATIM)
else()
    message(WARNING "cannot generate container_node_sizes.hpp, node size information will be unavailable")
endif()
