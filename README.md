# memory

[![Build Status](https://travis-ci.org/foonathan/memory.svg?branch=master)](https://travis-ci.org/foonathan/memory) [![Build status](https://ci.appveyor.com/api/projects/status/ef654yqyoqgvl472/branch/master?svg=true)](https://ci.appveyor.com/project/foonathan/memory/branch/master)

The C++ STL allocator model has various flaws. For example, they are fixed to a certain type, because they are almost necessarily required to be templates. So you can't easily share a single allocator for multiple types. In addition, you can only get a copy from the containers and not the original allocator object. At least with C++11 they are allowed to be stateful and so can be made object not instance based. But still, the model has many flaws.
Over the course of the years many solutions have been proposed. for example [EASTL]. This library is another. But instead of trying to change the STL, it works with the current implementation.

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

See `example/` for more.

## Installation

This library is designed to work as [CMake] subdirectory.
It is tested on GCC 4.7-4.9, Clang 3.4-3.5 and Visual Studio 2013. Newer versions should work too.

1. Fetch it, e.g. using [git submodules] `git submodule add https://github.com/foonathan/memory ext/memory` and `git submodule update --init --recursive`.

2. Call `add_subdirectory(ext/memory)` or whatever your local path is to make it available in CMake.

3. Simply call `target_link_libraries(your_target PUBLIC foonathan_memory)` to link this library and setups the include search path.

4. You need to activate C++11 at your target, if not already done, you can use [foonathan/compatibility] already available through `add_subdirectory()` and call `comp_target_features(your_target PUBLIC CPP11)`.

*Note: If during CMake you see an error message that compatibility is 
not on the newest version, run `git submodule update 
--recursive --remote` to force the compatiblity submodule of memory to 
update to the latest version.*

See http://foonathan.github.io/doc/memory/md_doc_installation.html for a detailed guide.

## Documentation

Full documentation can be found at http://foonathan.github.io/doc/memory.

A tutorial is also available at http://foonathan.github.io/doc/memory/md_doc_tutorial.html.

## RawAllocator

Below is the interface required for a `RawAllocator`, everything optional is marked:

```cpp
struct raw_allocator
{
    using is_stateful = std::integral_constant<bool, Value>; // optional, defaults to std::is_empty

    void* allocate_node(std::size_t size, std::size_t alignment); // required, allocation function
    void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept; // required, deallocation function

    void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment); // optional, forwards to node version
    void deallocate_array(void *ptr, std::size_t count, std::size_t size, std::size_t alignment) noexcept; // optional, forwards to node version

    std::size_t max_node_size() const; // optional, returns maximum value
    std::size_t max_array_size() const; // optional, forwards to max_node_size()
    std::size_t max_alignment() const; // optional, returns maximum fundamental alignment, i.e. alignof(std::max_align_t)
};
```

A `RawAllocator` only needs to be moveable, all `Allocator` classes are `RawAllocators` too.
Classes not providing the interface can specialize the `allocator_traits`, read more about [writing allocators here](http://foonathan.github.io/doc/memory/md_doc_writing_allocators.html) or about the technical details of the [concept here](http://foonathan.github.io/doc/memory/md_doc_concepts.html).

[EASTL]: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html
[CMake]: www.cmake.org
[git submodules]: http://git-scm.com/docs/git-submodule
[foonathan/compatibility]: hptts://github.com/foonathan/compatibility
