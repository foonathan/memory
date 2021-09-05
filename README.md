# memory

![Project Status](https://img.shields.io/endpoint?url=https%3A%2F%2Fwww.jonathanmueller.dev%2Fproject%2Fmemory%2Findex.json)
![Build Status](https://github.com/foonathan/memory/workflows/Main%20CI/badge.svg)
[![Code Coverage](https://codecov.io/gh/foonathan/memory/branch/master/graph/badge.svg?token=U6wnInlamY)](https://codecov.io/gh/foonathan/memory)

The C++ STL allocator model has various flaws. For example, they are fixed to a certain type, because they are almost necessarily required to be templates. So you can't easily share a single allocator for multiple types. In addition, you can only get a copy from the containers and not the original allocator object. At least with C++11 they are allowed to be stateful and so can be made object not instance based. But still, the model has many flaws.
Over the course of the years many solutions have been proposed, for example [EASTL]. This library is another. But instead of trying to change the STL, it works with the current implementation.

> |[![](https://www.jonathanmueller.dev/embarcadero-logo.png)](https://www.embarcadero.com/de/products/cbuilder/starter) | Sponsored by [Embarcadero C++Builder](https://www.embarcadero.com/de/products/cbuilder/starter). |
> |-------------------------------------|----------------|
>
> If you like this project, consider [supporting me](https://jonathanmueller.dev/support-me/).

## Features

New allocator concepts:

* a `RawAllocator` that is similar to an `Allocator` but easier to use and write
* a `BlockAllocator` that is an allocator for huge memory blocks

Several implementations:

* `heap_/malloc_/new_allocator`
* virtual memory allocators
* allocator using a static memory block located on the stack
* memory stack, `iteration_allocator`
* different memory pools
* a portable, improved `alloca()` in the form of `temporary_allocator`
* facilities for joint memory allocations: share a big memory block for the object
and all dynamic memory allocations for its members

Adapters, wrappers and storage classes:

* incredible powerful `allocator_traits` allowing `Allocator`s as `RawAllocator`s
* `std_allocator` to make a `RawAllocator` an `Allocator` again
* adapters for the memory resource TS
* `allocator_deleter` classes for smart pointers
* (optionally type-erased) `allocator_reference` and other storage classes
* memory tracking wrapper

In addition:

* container node size debuggers that obtain information about the node size of an STL container at compile-time to specify node sizes for pools
* debugging options for leak checking, double-free checks or buffer overflows
* customizable error handling routines that can work with exceptions disabled
* everything except the STL adapters works on a freestanding environment

## Basic example

```cpp
#include <algorithm>
#include <iostream>
#include <iterator>

#include <foonathan/memory/container.hpp> // vector, list, list_node_size
#include <foonathan/memory/memory_pool.hpp> // memory_pool
#include <foonathan/memory/smart_ptr.hpp> // allocate_unique
#include <foonathan/memory/static_allocator.hpp> // static_allocator_storage, static_block_allocator
#include <foonathan/memory/temporary_allocator.hpp> // temporary_allocator

// alias namespace foonathan::memory as memory for easier access
#include <foonathan/memory/namespace_alias.hpp>

template <typename BiIter>
void merge_sort(BiIter begin, BiIter end);

int main()
{
    using namespace memory::literals;

    // a memory pool RawAllocator
    // allocates a memory block - initially 4KiB - and splits it into chunks of list_node_size<int>::value big
    // list_node_size<int>::value is the size of each node of a std::list
    memory::memory_pool<> pool(memory::list_node_size<int>::value, 4_KiB);

    // just an alias for std::list<int, memory::std_allocator<int, memory::memory_pool<>>
    // a std::list using a memory_pool
    // std_allocator stores a reference to a RawAllocator and provides the Allocator interface
    memory::list<int, memory::memory_pool<>> list(pool);
    list.push_back(3);
    list.push_back(2);
    list.push_back(1);

    for (auto e : list)
        std::cout << e << ' ';
    std::cout << '\n';

    merge_sort(list.begin(), list.end());

    for (auto e : list)
        std::cout << e << ' ';
    std::cout << '\n';

    // allocate a std::unique_ptr using the pool
    // memory::allocate_shared is also available
    auto ptr = memory::allocate_unique<int>(pool, *list.begin());
    std::cout << *ptr << '\n';

    // static storage of size 4KiB
    memory::static_allocator_storage<4096u> storage;

    // a memory pool again but this time with a BlockAllocator
    // this controls the internal allocations of the pool itself
    // we need to specify the first template parameter giving the type of the pool as well
    // (node_pool is the default)
    // we use a static_block_allocator that uses the static storage above
    // all allocations will use a memory block on the stack
    using static_pool_t = memory::memory_pool<memory::node_pool, memory::static_block_allocator>;
    static_pool_t static_pool(memory::unordered_set_node_size<int>::value, 4096u, storage);

    // again, just an alias for std::unordered_set<int, std::hash<int>, std::equal_to<int>, memory::std_allocator<int, static_pool_t>
    // see why I wrote these? :D
    // now we have a hash set that lives on the stack!
    memory::unordered_set<int, static_pool_t> set(static_pool);

    set.insert(3);
    set.insert(2);
    set.insert(3); // running out of stack memory is properly handled, of course

    for (auto e : set)
        std::cout << e << ' ';
    std::cout << '\n';
}

// naive implementation of merge_sort using temporary memory allocator
template <typename BiIter>
void merge_sort(BiIter begin, BiIter end)
{
    using value_type = typename std::iterator_traits<BiIter>::value_type;

    auto distance = std::distance(begin, end);
    if (distance <= 1)
        return;

    auto mid = begin;
    std::advance(mid, distance / 2);

    // an allocator for temporary memory
    // is similar to alloca() but uses its own stack
    // this stack is thread_local and created on the first call to this function
    // as soon as the allocator object goes out of scope, everything allocated through it, will be freed
    auto alloc = memory::temporary_allocator();

    // alias for std::vector<value_type, memory::std_allocator<value_type, memory::temporary_allocator>>
    // a std::vector using a temporary_allocator
    memory::vector<value_type, memory::temporary_allocator> first(begin, mid, alloc),
                                                            second(mid, end, alloc);

    merge_sort(first.begin(), first.end());
    merge_sort(second.begin(), second.end());
    std::merge(first.begin(), first.end(), second.begin(), second.end(), begin);
}
```

See `example/` for more.

## Installation

This library can be used as [CMake] subdirectory.
It is tested on GCC 4.8-5.0, Clang 3.5 and Visual Studio 2013. Newer versions should work too.

1. Fetch it, e.g. using [git submodules] `git submodule add https://github.com/foonathan/memory ext/memory` and `git submodule update --init --recursive`.

2. Call `add_subdirectory(ext/memory)` or whatever your local path is to make it available in CMake.

3. Simply call `target_link_libraries(your_target PUBLIC foonathan_memory)` to link this library and setups the include search path and compilation options.

You can also install the library:

1. Run `cmake -DCMAKE_BUILD_TYPE="buildtype" -DFOONATHAN_MEMORY_BUILD_EXAMPLES=OFF -DFOONATHAN_MEMORY_BUILD_TESTS=OFF .` inside the library sources.

2. Run `cmake --build . -- install` to install the library under `${CMAKE_INSTALL_PREFIX}`.

3. Repeat 1 and 2 for each build type/configuration you want to have (like `Debug`, `RelWithDebInfo` and `Release` or custom names).

To use an installed library:

4. Call `find_package(foonathan_memory major.minor REQUIRED)` to find the library.

5. Call `target_link_libraries(your_target PUBLIC foonathan_memory)` to link to the library and setup all required options.
  
See https://memory.foonathan.net/md_doc_installation.html for a detailed guide.

## Documentation

Full documentation can be found at https://memory.foonathan.net/.

A tutorial is also available at https://memory.foonathan.net/md_doc_tutorial.html.

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
Classes not providing the interface can specialize the `allocator_traits`, read more about [writing allocators here](https://memory.foonathan.net/md_doc_writing_allocators.html) or about the technical details of the [concept here](https://memory.foonathan.net/md_doc_concepts.html).

## Acknowledgements

This project is greatly supported by my [patrons](https://patreon.com/foonathan).
In particular thanks to the individual supporters:

* Kaido Kert

And big thanks to the contributors as well:

* @Guekka
* @Manu343726
* @MiguelCompany
* @asobhy-qnx
* @bfierz
* @cho3
* @gabyx
* @j-carl
* @kaidokert
* @maksqwe
* @maksqwe
* @moazzamak
* @moazzamak
* @myd7349
* @myd7349
* @nicolastagliani
* @quattrinili
* @razr
* @roehling
* @seanyen
* @wtsnyder
* @zhouchengming1
* @jwdevel

[EASTL]: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html
[CMake]: www.cmake.org
[git submodules]: http://git-scm.com/docs/git-submodule
[foonathan/compatibility]: hptts://github.com/foonathan/compatibility
