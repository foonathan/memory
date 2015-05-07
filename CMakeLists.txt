if (BIICODE)
    include(${CMAKE_HOME_DIRECTORY}/biicode.cmake)
    include(biicode/cmake/tools)
else()
    cmake_minimum_required(VERSION 3.0)
    project(foonathan_memory)
endif()

set(version_major 0 CACHE INTERNAL "")
set(version_minor 2 CACHE INTERNAL "")

set(FOONATHAN_MEMORY_DEFAULT_ALLOCATOR heap_allocator CACHE STRING
    "the default implementation allocator for higher-level ones")
option(FOONATHAN_MEMORY_THREAD_SAFE_ADAPTER "whether or not raw_allocator_adapter is thread safe by default" ON)

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
            detail/memory_stack.hpp
            allocator_adapter.hpp
            allocator_traits.hpp
            config.hpp
            default_allocator.hpp
            heap_allocator.cpp
            heap_allocator.hpp
            new_allocator.cpp
            new_allocator.hpp
            pool_allocator.hpp
            pool_collection.cpp
            pool_collection.hpp
            pool_type.hpp
            raw_allocator_base.hpp
            smart_ptr.hpp
            stack_allocator.hpp
            std_allocator_base.hpp
            temporary_allocator.cpp
            temporary_allocator.hpp
            tracking.hpp
    CACHE INTERNAL "")
    
    add_library(foonathan_memory ${src})
    add_executable(foonathan_memory_example_allocator ${src} example/allocator.cpp)
    add_executable(foonathan_memory_example_smart_ptr ${src} example/smart_ptr.cpp)
    add_executable(foonathan_memory_example_temporary ${src} example/temporary.cpp)
    
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
