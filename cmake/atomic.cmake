# check for atomic library, which is needed on some architectures
include(CheckCXXSourceCompiles)
function(check_working_cxx_atomic varname)
	check_cxx_source_compiles("
	#include <atomic>
	std::atomic<bool> x;
	int main() {
		bool y = false;
		return !x.compare_exchange_strong(y, true);
	}" ${varname})
endfunction()
check_working_cxx_atomic(HAVE_CXX_ATOMIC)
if(NOT HAVE_CXX_ATOMIC)
	set(old_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
	list(APPEND CMAKE_REQUIRED_LIBRARIES "-latomic")
	check_working_cxx_atomic(NEED_LIBRARY_FOR_CXX_ATOMIC)
	set(CMAKE_REQUIRED_LIBRARIES "${old_CMAKE_REQUIRED_LIBRARIES}")
	if(NOT NEED_LIBRARY_FOR_CXX_ATOMIC)
		message(FATAL_ERROR "Host compiler does not support std::atomic")
	endif()
endif()
