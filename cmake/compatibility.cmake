# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# compatibility.cmake - various necessary compatibility checks
# note: only include it in memory's top-level CMakeLists.txt

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/comp/comp_base.cmake)

# dummy library running the required tests
add_library(_foonathan_memory_comp_runner INTERFACE)
set(_foonathan_memory_comp_include_path "${CMAKE_CURRENT_BINARY_DIR}")
comp_target_features(_foonathan_memory_comp_runner INTERFACE
        cpp11_lang/alignas cpp11_lang/alignof cpp11_lang/constexpr cpp11_lang/noexcept cpp11_lang/thread_local
        cpp11_lib/get_new_handler cpp11_lib/max_align_t cpp11_lib/mutex
        ts/pmr
        env/exception_support env/hosted_implementation
        ext/clz
        PREFIX "FOONATHAN_" NAMESPACE "foonathan_comp"
        INCLUDE_PATH ${_foonathan_memory_comp_include_path}
        NOFLAGS)
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/foonathan DESTINATION include/comp)

function(_foonathan_use_comp target)
    target_compile_options(${target} PUBLIC ${_foonathan_memory_comp_runner_COMP_COMPILE_OPTIONS})
    target_include_directories(${target} PUBLIC $<BUILD_INTERFACE:${_foonathan_memory_comp_include_path}>
                                                $<INSTALL_INTERFACE:include/comp>)
endfunction()
