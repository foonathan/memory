# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds actual library

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
        ${header_path}/aligned_allocator.hpp
        ${header_path}/allocator_adapter.hpp
        ${header_path}/allocator_traits.hpp
        ${header_path}/config.hpp
        ${header_path}/container.hpp
        ${header_path}/debugging.hpp
        ${header_path}/default_allocator.hpp
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

# only apply features if not using MSVC, not supported there on cmake 3.1 and unnecessary
set(cpp_features
        cxx_alias_templates
        cxx_auto_type
        cxx_defaulted_functions
        cxx_defaulted_move_initializers
        cxx_delegating_constructors
        cxx_deleted_functions
        cxx_extended_friend_declarations
        cxx_nonstatic_member_init
        cxx_nullptr
        cxx_range_for
        cxx_right_angle_brackets
        cxx_static_assert
        cxx_strong_enums
        cxx_trailing_return_types
        cxx_uniform_initialization
        cxx_variadic_templates
        cxx_template_template_parameters)

if(BIICODE)
    target_include_directories(${BII_BLOCK_TARGET} INTERFACE ${CMAKE_CURRENT_BINARY_DIR}) # for config_impl.hpps
    if(NOT MSVC)
        target_compile_features(${BII_BLOCK_TARGET} INTERFACE ${cpp_features})
    endif()

    if(BII_LIB_SRC)
        target_include_directories(${BII_LIB_TARGET} INTERFACE ${header_path})
    endif()
else()
    add_library(foonathan_memory ${header} ${src})
    target_include_directories(foonathan_memory PRIVATE ${header_path})
    target_include_directories(foonathan_memory PUBLIC ${CMAKE_CURRENT_BINARY_DIR}) # for config_impl.hpp
    target_include_directories(foonathan_memory INTERFACE ${FOONATHAN_MEMORY_INCLUDE_DIR}) # for other targets using it
    if(NOT MSVC)
        target_compile_features(foonathan_memory PUBLIC ${cpp_features})
    endif()
endif()
