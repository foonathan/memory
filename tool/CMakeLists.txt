# Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
# SPDX-License-Identifier: Zlib

# builds tools

add_executable(foonathan_memory_node_size_debugger test_types.hpp node_size_debugger.hpp node_size_debugger.cpp)
if (MSVC)
    target_compile_options(foonathan_memory_node_size_debugger PRIVATE "/bigobj")
endif()
target_compile_definitions(foonathan_memory_node_size_debugger PUBLIC
                           VERSION="$CACHE{FOONATHAN_MEMORY_VERSION_MAJOR}.$CACHE{FOONATHAN_MEMORY_VERSION_MINOR}")
set_target_properties(foonathan_memory_node_size_debugger PROPERTIES OUTPUT_NAME nodesize_dbg)
if(NOT MSVC)
target_compile_features(foonathan_memory_node_size_debugger PUBLIC cxx_constexpr)
endif()

install(TARGETS foonathan_memory_node_size_debugger EXPORT foonathan_memoryTargets
                                                    RUNTIME DESTINATION ${FOONATHAN_MEMORY_RUNTIME_INSTALL_DIR})
