# defines a variable CXX11_FLAG that enables C++11 support

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CXX11_FLAG -std=c++11)
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CXX11_FLAG -std=c++0x)
else()
    set(CXX11_FLAG)
endif()
