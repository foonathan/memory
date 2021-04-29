# Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# builds tools

add_executable(foonathan_memory_node_size_debugger test_types.hpp node_size_debugger.hpp node_size_debugger.cpp)
if (CMAKE_CROSSCOMPILING)
    # statically link when cross compiling so emulator doesn't need library paths
    if (MSVC)
        set_target_properties(foonathan_memory_node_size_debugger PROPERTIES LINK_FLAGS "/WHOLEARCHIVE")
    else()
        set_target_properties(foonathan_memory_node_size_debugger PROPERTIES LINK_FLAGS "-static")
    endif()
endif()
if (MSVC)
    target_compile_options(foonathan_memory_node_size_debugger PRIVATE "/bigobj")
endif()
target_compile_definitions(foonathan_memory_node_size_debugger PUBLIC
                           VERSION="${FOONATHAN_MEMORY_VERSION_MAJOR}.${FOONATHAN_MEMORY_VERSION_MINOR}")
set_target_properties(foonathan_memory_node_size_debugger PROPERTIES OUTPUT_NAME nodesize_dbg)
if(NOT MSVC)
target_compile_features(foonathan_memory_node_size_debugger PUBLIC cxx_constexpr)
endif()

install(TARGETS foonathan_memory_node_size_debugger EXPORT foonathan_memoryTargets
                                                    RUNTIME DESTINATION ${FOONATHAN_MEMORY_RUNTIME_INSTALL_DIR})
