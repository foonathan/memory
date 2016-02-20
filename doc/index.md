This is the documentation of `foonathan/memory`.

For a quick start, read the [Tutorial] or skim the examples at the [Github page].
The concepts of this library are defined are [here](md_doc_concepts.html).

## Features

* new `RawAllocator` concept that is similar to an `Allocator` but easier to use and write
* implementations of it like memory pools,  and other allocator classes
* a portable, improved `alloca()` in the form of `temporary_allocator`
* various adapter classes, e.g. for memory tracking or type-erased references
* std_allocator class that converts a RawAllocator to an Allocator, so they can be used everywhere an Allocator is accepted
* allocator_deleter clases to have deleters that use a RawAllocator
* container node size debuggers that obtain information about the node size of an STL container at compile-time to specify node sizes for pools
* customizable error handling routines that can work with exceptions disabled
* debugging options for leak checking, double-free checks or buffer overflows
* most parts can work on a freestanding implementation

## Basic example

```cpp
#include <algorithm>
#include <iostream>
#include <iterator>

#include <memory/container.hpp>
#include <memory/memory_pool.hpp>
#include <memory/smart_ptr.hpp>
#include <memory/temporary_allocator.hpp>

template <typename BiIter>
void merge_sort(BiIter begin, BiIter end);

int main()
{
    memory::memory_pool<> pool(memory::list_node_size<int>::value, 4096u);

    // alias for std::list<int, memory::std_allocator<int, memory::memory_pool<>>
    memory::list<int, memory::memory_pool<>> list(pool);
    list.push_back(3);
    list.push_back(2);
    list.push_back(1);

    merge_sort(list.begin(), list.end());

    // a unique_ptr using the pool
    auto ptr = memory::allocate_unique<int>(pool, *list.begin());
    std::cout << *ptr << '\n';
}

template <typename BiIter>
void merge_sort(BiIter begin, BiIter end)
{
    using value_type = typename std::iterator_traits<BiIter>::value_type;

    auto distance = std::distance(begin, end);
    if (distance <= 1)
        return;

    auto mid = begin;
    std::advance(mid, distance / 2);

    auto alloc = memory::make_temporary_allocator();

    // alias for std::vector<value_type, memory::std_allocator<value_type, memory::temporary_allocator>>
    memory::vector<value_type, memory::temporary_allocator> first(begin, mid, alloc),
                                                            second(mid, end, alloc);

    merge_sort(first.begin(), first.end());
    merge_sort(second.begin(), second.end());
    std::merge(first.begin(), first.end(), second.begin(), second.end(), begin);
}
```

## Installation

This library can be used as [CMake] subdirectory.
It is tested on GCC 4.7-4.9, Clang 3.4-3.5 and Visual Studio 2013. Newer versions should work too.

1. Fetch it, e.g. using [git submodules] `git submodule add https://github.com/foonathan/memory ext/memory` and `git submodule update --init --recursive`.

2. Call `add_subdirectory(ext/memory)` or whatever your local path is to make it available in CMake.

3. Simply call `target_link_libraries(your_target PUBLIC foonathan_memory)` to link this library and setups the include search path.

4. You need to activate C++11 at your target, if not already done, you can use [foonathan/compatibility] already available through `add_subdirectory()` and call `comp_target_features(your_target PUBLIC CPP11)`.

*Note: If during CMake you see an error message that compatibility is 
not on the newest version, run `git submodule update 
--recursive --remote` to force the compatiblity submodule of memory to 
update to the latest version.*

You can also install the library:

1. Run `cmake -DCMAKE_BUILD_TYPE="buildtype" -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF .` inside the library sources.

2. Run `cmake --build . -- install` to install the library under `${CMAKE_INSTALL_PREFIX}`.

3. Repeat 1 and 2 for each build type/configuration you want to have (like `Debug`, `RelWithDebInfo` and `Release` or custom names).

The use an installed library:

4. Call `find_package(foonathan_memory major.minor REQUIRED)` to find the library.

5. Call `target_link_libraries(your_target PUBLIC foonathan_memory)` and activate C++11 to link to the library.

See http://foonathan.github.io/doc/memory/md_doc_installation.html for a detailed guide.

## About this documentation

This documentation is written in a similar way as the C++ standard itself, although not that formal.

Concepts are documented using the names of the template parameters, for example the following class:

~~~{.cpp}
template <class Tracker, class RawAllocator>
class tracked_allocator;
~~~

It takes two template parameters, the first must model the [Tracker] concept, the second the [RawAllocator] concept.

Unless explicitly stated otherwise, it is not allowed to call a function that modifies state from two different threads.
Functions that modify state are non-`const` member functions, functions taking a non-`const` reference to objects
or functions where it is explictly documented that they change some hidden state.

If a function is documented as `noexcept`, it does not throw anything.
Otherwise it has a *Throws:* clause specifying what it throws, or if it is a forwarding function, the information can be found there (see below).

If a class is described as [RawAllocator] it automatically has certain semantically information which are not explictly mentioned.
This is especially true for the member functions of an [allocator_traits] specialization.

If a function is described as returning the value of another function or forwarding to it,
it implicitly has the requirements and effects from the called function and can also throw the same things.

[Tutorial]: md_doc_tutorial.html
[Github page]: https://github.com/foonathan/memory/
[Tracker]: md_doc_concepts.html#concept_tracker
[RawAllocator]: md_doc_concepts.html#concept_rawallocator
[allocator_traits]: \ref foonathan::memory::allocator_traits
