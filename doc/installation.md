# Installation

This library is designed to be used with [CMake]'s `add_subdirectory()` command.

## 0. Requirements

* git
* CMake version 3.1 or higher
* GCC 4.7 or higher, or clang 3.4 or higher or Visual Studio 12 or higher
* the project that wants to use it already uses CMake

## 1. Fetching

It is recommended to setup a [git submodule] inside your project.
Simply run:

1. `git submodule add https://github.com/foonathan/memory ext/memory`. This will clone the latest commit into a local directory `ext/memory` and registers it as a submodule.
2. `git submodule update --init --recursive`. This will fetches the latest commits from all submodules `memory` itself is using.

If you later want to update your local copy to the latest version, simply run: `git submodule update --recursive --remote`.

*Note: Run this command also if you get an error message during CMake 
telling you that a newer version of compatibility is required. This will 
update the compatibility submodule to its newest version.*

## 2. CMake Setup

I am assuming that there is a local copy of the library source files under the path `ext/memory`.

In your `CMakeLists.txt` place a call to `add_subdirectory(ext/memory)`.
This will make all targets of `memory` available inside your CMakeLists.txt.
If you want to configure the library, add `set(option value CACHE INTERNAL "" FORCE)` before the `add_subdirectory()` command.
See [options] for a list of all configuration options and CMake variables.

Now the targets is available and to use the library in your application, call to `target_link_libraries(target foonathan_memory)`.
This will also setups the include search path of the compiler.

The library requires C++11 support. If it is not already enabled for your target, you can use my [compatibility library].
It provides an advanced `target_compile_features()` functionality with automated workaround code for missing features,
but also a simple way to activate C++11. Just call `comp_target_features(target PUBLIC CPP11)`.
This function is already available through the call to `add_subdirectory()` since `memory` is using it, too.

## 3. Code Usage

In your code you simply need to include the appropriate headers and you are good to got.
By default, everything is under the subdirectory `memory/`, so write `#include <memory/heap_allocator.hpp>` to use the [heap_allocator].
Also by default a namespace alias `memory` is available which allows access, i.e. write `memory::heap_allocator`.

[CMake]: www.cmake.org
[git submodule]: http://git-scm.com/docs/git-submodule
[compatibility library]: https://github.com/foonathan/compatibility
[heap_allocator]: \ref foonathan::memory::heap_allocator
[options]: md_doc_options.html
