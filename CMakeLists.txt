if (BIICODE)
    include(${CMAKE_HOME_DIRECTORY}/biicode.cmake)
    include(biicode/cmake/tools)
else()
    cmake_minimum_required(VERSION 3.0)
    project(foonathan_memory)
endif()

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG(-std=c++11 cpp11_flag)
if (cpp11_flag)
    set(CMAKE_REQUIRED_FLAGS "-std=c++11")
else()
    CHECK_CXX_COMPILER_FLAG(-std=c++0x cpp0x_flag)
    if (cpp0x_flag)
        set(CMAKE_REQUIRED_FLAGS "-std=c++0x")
    endif(cpp0x_flag)
endif(cpp11_flag)

CHECK_CXX_SOURCE_COMPILES("int main() {int i = alignof(int);}" comp_alignof)
CHECK_CXX_SOURCE_COMPILES("#include <cstddef>
                          using namespace std;
                          int main() {max_align_t val;}" comp_max_align)
CHECK_CXX_SOURCE_COMPILES("#include <new>
                          int main() {auto handler = std::get_new_handler();}" comp_new_handler)
CHECK_CXX_SOURCE_COMPILES("thread_local int i; int main() {}" comp_thread_local)
CHECK_CXX_SOURCE_COMPILES("constexpr auto foo = 1; int main(){}" comp_constexpr)
CHECK_CXX_SOURCE_COMPILES("void foo() noexcept {} int main(){}" comp_noexcept)

option(FOONATHAN_IMPL_HAS_ALIGNOF "whether or not alignof is available" ${comp_alignof})
option(FOONATHAN_IMPL_HAS_MAX_ALIGN "whether or not std::max_align_t is available" ${comp_max_align})
option(FOONATHAN_IMPL_HAS_GET_NEW_HANDLER "whether or not std::get_new_handler() is available" ${comp_new_handler})
option(FOONATHAN_IMPL_HAS_THREAD_LOCAL "whether or not thread_local is available" ${comp_thread_local})
option(FOONATHAN_IMPL_HAS_CONSTEXPR "whether or not constexpr is available" ${comp_constexpr})
option(FOONATHAN_IMPL_HAS_NOEXCEPT "whether or not noexcept is available" ${comp_noexcept})

set(version_major 0 CACHE INTERNAL "")
set(version_minor 2 CACHE INTERNAL "")

set(FOONATHAN_MEMORY_DEFAULT_ALLOCATOR heap_allocator CACHE STRING
    "the default implementation allocator for higher-level ones")
option(FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE "whether or not allocator_reference is thread safe by default" ON)
option(FOONATHAN_MEMORY_DEBUG_FILL "whether or not filling memory with magic values for debugging purposes is enabled" ON)
set(FOONATHAN_MEMORY_DEBUG_FENCE 0 CACHE INTEGER
    "the amount of memory used to help catching overflow errors" )
option(FOONATHAN_MEMORY_DEBUG_LEAK_CHECK "whether or not leak checking is active" OFF)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/config_impl.hpp")

if (BIICODE)
    ADD_BIICODE_TARGETS()
    ACTIVATE_CPP11()
    target_include_directories(${BII_BLOCK_TARGET} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
else()
    set(src
            detail/align.hpp
            detail/block_list.cpp
            detail/block_list.hpp
            detail/free_list.cpp
            detail/free_list.hpp
            detail/memory_stack.cpp
            detail/memory_stack.hpp
            detail/small_free_list.cpp
            detail/small_free_list.hpp
            aligned_allocator.hpp
            allocator_adapter.hpp
            allocator_traits.hpp
            config.hpp
            container.hpp
            debugging.cpp
            debugging.hpp
            default_allocator.hpp
            heap_allocator.cpp
            heap_allocator.hpp
            new_allocator.cpp
            new_allocator.hpp
            pool_allocator.hpp
            pool_collection.cpp
            pool_collection.hpp
            pool_type.hpp
            pool_type.cpp
            raw_allocator_base.hpp
            smart_ptr.hpp
            stack_allocator.hpp
            std_allocator_base.hpp
            temporary_allocator.cpp
            temporary_allocator.hpp
            threading.hpp
            tracking.hpp
    CACHE INTERNAL "")

    add_library(foonathan_memory ${src})
    add_executable(foonathan_memory_example_allocator example/allocator.cpp)
    add_executable(foonathan_memory_example_smart_ptr example/smart_ptr.cpp)
    add_executable(foonathan_memory_example_temporary example/temporary.cpp)

    target_link_libraries(foonathan_memory_example_allocator PUBLIC foonathan_memory)
    target_link_libraries(foonathan_memory_example_smart_ptr PUBLIC foonathan_memory)
    target_link_libraries(foonathan_memory_example_temporary PUBLIC foonathan_memory)

    set(targets foonathan_memory
                foonathan_memory_example_allocator
                foonathan_memory_example_smart_ptr
                foonathan_memory_example_temporary
        CACHE INTERNAL "")

    set_target_properties(${targets} PROPERTIES CXX_STANDARD 11)

    foreach(target ${targets})
        target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    endforeach()
endif()
