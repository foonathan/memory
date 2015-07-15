# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds tools

if(BIICODE)
    return()
endif()

add_executable(foonathan_memory_node_size_debugger node_size_debugger.hpp node_size_debugger.cpp)

if(NOT MSVC)
        target_compile_features(foonathan_memory_node_size_debugger PUBLIC cxx_variadic_templates)
endif()
