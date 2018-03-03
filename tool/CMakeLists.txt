# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds tools

add_executable(foonathan_memory_node_size_debugger test_types.hpp node_size_debugger.hpp node_size_debugger.cpp)
_foonathan_use_comp(foonathan_memory_node_size_debugger)
comp_target_features(foonathan_memory_node_size_debugger PUBLIC CPP11)
if (MSVC)
    target_compile_options(foonathan_memory_node_size_debugger PRIVATE "/bigobj")
endif()
target_compile_definitions(foonathan_memory_node_size_debugger PUBLIC
                           VERSION="${FOONATHAN_MEMORY_VERSION_MAJOR}.${FOONATHAN_MEMORY_VERSION_MINOR}")
set_target_properties(foonathan_memory_node_size_debugger PROPERTIES OUTPUT_NAME nodesize_dbg)

install(TARGETS foonathan_memory_node_size_debugger EXPORT foonathan_memoryTargets
                                                    RUNTIME DESTINATION ${FOONATHAN_MEMORY_RUNTIME_INSTALL_DIR})