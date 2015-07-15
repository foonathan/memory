# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds actual library

include(../cmake/cpp11.cmake)

# configure config file
configure_file("config.hpp.in" "${CMAKE_CURRENT_BINARY_DIR}/config_impl.hpp")

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
        ${header_path}/new_allocator.hpp
        ${header_path}/pool_allocator.hpp
        ${header_path}/pool_collection.hpp
        ${header_path}/pool_type.hpp
        ${header_path}/smart_ptr.hpp
        ${header_path}/stack_allocator.hpp
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
    target_include_directories(${BII_BLOCK_TARGET} INTERFACE ${CMAKE_CURRENT_BINARY_DIR}) # for config_impl.hpps
    target_link_libraries(${BII_BLOCK_TARGET} INTERFACE foonathan_cpp11)

    if(BII_LIB_SRC)
        target_include_directories(${BII_LIB_TARGET} INTERFACE ${header_path})
    endif()
else()
    add_library(foonathan_memory ${header} ${src})
    target_include_directories(foonathan_memory PRIVATE ${header_path})
    target_include_directories(foonathan_memory PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # for config_impl.hpp
    target_include_directories(foonathan_memory INTERFACE ${FOONATHAN_MEMORY_INCLUDE_DIR}) # for other targets using it
    target_link_libraries(foonathan_memory PUBLIC foonathan_cpp11)
endif()
