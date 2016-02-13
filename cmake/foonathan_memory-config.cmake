# Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# package configuration file

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
    set(lib_dest ${SELF_DIR}/debug)
elseif(${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    set(lib_dest ${SELF_DIR}/relwithdebinfo)
else()
    set(lib_dest ${SELF_DIR}/release)
endif()

include(${lib_dest}/foonathan_memory.cmake)
