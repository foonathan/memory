# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds tools

include(../cmake/cpp11.cmake)

add_executable(foonathan_memory_node_size_debugger node_size_debugger.hpp node_size_debugger.cpp)
target_link_libraries(foonathan_memory_node_size_debugger PUBLIC foonathan_cpp11)
target_compile_definitions(foonathan_memory_node_size_debugger PUBLIC
                           VERSION="${FOONATHAN_MEMORY_VERSION_MAJOR}.${FOONATHAN_MEMORY_VERSION_MINOR}")
set_target_properties(foonathan_memory_node_size_debugger PROPERTIES
                        OUTPUT_NAME nodesize_dbg)