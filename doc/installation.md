# Installation {#md_doc_installation}

This library can either be used [CMake]'s `add_subdirectory()` command or installed globally.

## Requirements

* git
* CMake version 3.1 or higher
* GCC 4.9 or higher, or clang 3.5 or higher or Visual Studio 14 or higher

## As subdirectory of your project

### 1. Fetching

It is recommended to setup a [git submodule] inside your project.
Simply run:

1. `git submodule add https://github.com/foonathan/memory ext/memory`. This will clone the latest commit into a local directory `ext/memory` and registers it as a submodule.
2. `git submodule update --init --recursive`. This will fetches the latest commits from all submodules `memory` itself is using.

If you later want to update your local copy to the latest version, simply run: `git submodule update --recursive --remote`.

### 2. CMake Setup

I am assuming that there is a local copy of the library source files under the path `ext/memory`.

In your `CMakeLists.txt` place a call to `add_subdirectory(ext/memory)`.
This will make all targets of `memory` available inside your CMakeLists.txt.
If you want to configure the library, add `set(option value CACHE INTERNAL "" FORCE)` before the `add_subdirectory()` command.
See [options] for a list of all configuration options and CMake variables.

Now the targets is available and to use the library in your application, call to `target_link_libraries(target foonathan_memory)`.
This will also setups the include search path of the compiler, as well as other required flags.

### 3. Code Usage

In your code you simply need to include the appropriate headers and you are good to got.
Everything is under the subdirectory `foonathan/memory` so write `#include <foonathan/memory/heap_allocator.hpp>` to use the [heap_allocator].

## Installing the library

Download or clone the source for the library version you want to install.
You can build the library inside the source directory, it will not be needed after the installation.

For each build type, run `cmake -DCMAKE_BUILD_TYPE="buildtype" -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF .` with possible other [options] to configure, then simply `cmake --build . -- install` to build and install the library.
This will install the header files under `${CMAKE_INSTALL_PREFIX}/include/foonathan_memory-${major}.${minor}`, the tools under `${CMAKE_INSTALL_PREFIX}/bin` and the build artifacts under `${CMAKE_INSTALL_PREFIX}/lib/foonathan_memory-${major}.${minor}`. 
By default, `${CMAKE_INSTALL_PREFIX}` is `/usr/local` under UNIX and `C:/Program Files` under Windows,
installation may require `sudo`/administrative privileges.

It is recommended that you install the library for the `Debug`, `RelWithDebInfo` and `Release` build types.
Each build type allows different CMake configurations and compiler flags, you can also create your own.

## Using an installed library (CMake)

After you've installed the library, all you need to call is `find_package(foonathan_memory major.minor REQUIRED)` to find it.
This will look for a library installation of a compatible version and the same build type as your project,
i.e. if you compile under build type `Debug`, it will also match the `Debug` library.
This *should* work without your help, otherwise it will tell you what to do.

A `0.x` version requires an exact match in the call to `find_package()`, otherwise a library with same major version and a higher minor version is also compatible.
If you want only exact version matches add the `EXACT` flag to `find_package()`.

If a right library version/configuration cannot be found, this is an error due to the `REQUIRED`.
If this is not what you want, leave it out and do conditionals based on the CMake variable `foonathan_memory_FOUND`.
In the source code, all targets linking to the library have the macro `FOONATHAN_MEMORY` defined automatically,
as `FOONATHAN_MEMORY_VERSION_MAJOR/MINOR`.
Use conditional compilation with them.

After that you can link to the library by calling `target_link_libraries(your_target PUBLIC foonathan_memory)`.
This setups everything needed.

Then simply include the headers, everything is under the subdirectory `foonathan/memory` so write `#include <foonathan/memory/heap_allocator.hpp>` to use the [heap_allocator].

## Using an installed library (other buildsystems)

To use the library with other build-systems, add `${CMAKE_INSTALL_PREFIX}/include/foonathan_memory-${major}.${minor}` and `${CMAKE_INSTALL_PREFIX}/lib/foonathan_memory-${major}.${minor}/${CMAKE_BUILD_TYPE}` to your include directories path.
Link to the library file in `${CMAKE_INSTALL_PREFIX}/lib/foonathan_memory-${major}.${minor}/${CMAKE_BUILD_TYPE}` and enable the right C++ standard for your configuration.

You should also globally define the `FOONATHAN_MEMORY` macro as `1` and the `FOONATHAN_MEMORY_VERSION_MAJOR/MINOR` macros as the corresponding values.

[CMake]: www.cmake.org
[git submodule]: http://git-scm.com/docs/git-submodule
[compatibility library]: https://github.com/foonathan/compatibility
[heap_allocator]: \ref foonathan::memory::heap_allocator
[options]: md_doc_options.html
