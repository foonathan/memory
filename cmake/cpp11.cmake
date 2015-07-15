# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# cpp11.cmake - interface target to easily activate C++11

if(TARGET foonathan_cpp11)
    return()
endif()

# set of C++11 language features I am using
set(cpp11_features
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

# interface library, link to it, to gain C++11 support
add_library(foonathan_cpp11 INTERFACE)

# only apply features if not using MSVC, not supported there on cmake 3.1 and unnecessary
if(NOT MSVC)
    target_compile_features(foonathan_cpp11 INTERFACE ${cpp11_features})
endif()
