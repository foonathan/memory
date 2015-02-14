if (BIICODE)
    include(${CMAKE_HOME_DIRECTORY}/biicode.cmake)
    include(biicode/cmake/tools)
    INIT_BIICODE_BLOCK()
else()
    cmake_minimum_required(VERSION 3.0)
    project(foonathan_memory)
endif()

set(header
            detail/block_list.hpp
            detail/free_list.hpp
            allocator_adapter.hpp
            heap_allocator.hpp
            new_allocator.hpp
            pool.hpp
            pool_allocator.hpp
            pool_collection.hpp
            raw_allocator_base.hpp
            stack_allocator.hpp
            std_allocator_base.hpp
            tracking.hpp
    CACHE INTERNAL "")
set(src
        detail/block_list.cpp
        detail/free_list.cpp
        heap_allocator.cpp
        new_allocator.cpp
 #       pool_collection.cpp
        tracking.cpp
    CACHE INTERNAL "")

if (BIICODE)
    set(BII_LIB_SRC ${src} ${header}) # force generation of library target, otherwise it will be INTERFACE only
    ADD_BIICODE_TARGETS()
    set(lib_target ${BII_LIB_TARGET} CACHE INTERNAL "")
    set(targets ${BII_BLOCK_TARGETS} CACHE INTERNAL "")
else()        
    add_library(foonathan_memory ${src} ${header})
    add_executable(memory ${src} ${header} example/main.cpp)
    
    set(lib_target foonathan_memory CACHE INTERNAL "")
    set(targets ${lib_target} memory CACHE INTERNAL "")
endif()

set_target_properties(${targets} PROPERTIES CXX_STANDARD 11)
set_target_properties(${targets} PROPERTIES CXX_STANDARD_REQUIRED ON)

foreach(target ${targets})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    if (BIICODE)
        ACTIVATE_CPP11(${target})
    endif()
endforeach()
