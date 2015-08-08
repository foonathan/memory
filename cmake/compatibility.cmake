# Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

# compatibility.cmake - various necessary compatibility checks

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/comp/comp_base.cmake)

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

check_cxx_compiler_flag(-std=c++11 has_cpp11_flag)
if(has_cpp11_flag)
    set(CMAKE_REQUIRED_FLAGS "-std=c++11")
else()
    check_cxx_compiler_flag(-std=c++0x has_cpp0x_flag)
    if(has_cpp0x_flag)
        set(CMAKE_REQUIRED_FLAGS "-std=c++0x")
    endif()
endif()

# tests if code compiles and provides override option for result
# creates internal variable has_${name} and option FOONATHAN_HAS_${name}
macro(foonathan_check_feature code name)
    check_cxx_source_compiles("${code}" has_${name})
    if(has_${name})
        option(FOONATHAN_HAS_${name} "whether or not ${name} is available" ON)
    else()
        option(FOONATHAN_HAS_${name} "whether or not ${name} is available" OFF)
    endif()
endmacro()

foonathan_check_feature("int main() {int i = alignof(int);}" ALIGNOF)
foonathan_check_feature("int main() {constexpr auto foo = 1;}" CONSTEXPR)
foonathan_check_feature("void foo() noexcept {} int main(){}" NOEXCEPT)
foonathan_check_feature("int main() {thread_local int i;}" THREAD_LOCAL)

# note: using namespace std; important here, as it might not be in namespace std
foonathan_check_feature("#include <cstddef>
                       using namespace std;
                       int main() {max_align_t val;}" MAX_ALIGN_T)

foonathan_check_feature("#include <new>
                        int main() {auto handler = std::get_new_handler();}" GET_NEW_HANDLER)
