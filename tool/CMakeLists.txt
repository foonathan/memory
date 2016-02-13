# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds tools

add_executable(foonathan_memory_node_size_debugger node_size_debugger.hpp node_size_debugger.cpp)
comp_target_features(foonathan_memory_node_size_debugger PRIVATE CPP11)
target_compile_definitions(foonathan_memory_node_size_debugger PUBLIC
                           VERSION="${FOONATHAN_MEMORY_VERSION_MAJOR}.${FOONATHAN_MEMORY_VERSION_MINOR}")
set_target_properties(foonathan_memory_node_size_debugger PROPERTIES OUTPUT_NAME nodesize_dbg)

install(TARGETS foonathan_memory_node_size_debugger EXPORT foonathan_memory DESTINATION ${tool_dest})