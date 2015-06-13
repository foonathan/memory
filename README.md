# memory
[![Build Status](https://webapi.biicode.com/v1/badges/foonathan/foonathan/memory/master)](https://www.biicode.com/foonathan/memory)

The C++ STL allocator model has various flaws. For example, they are fixed to a certain type, because they are almost necessarily required to be templates. So you can't easily share a single allocator for multiple types. In addition, you can only get a copy from the containers and not the original allocator object. At least with C++11 they are allowed to be stateful and so can be made object not instance based. But still, the model has many flaws.
Over the course of the years many solutions have been proposed. for example [EASTL]. This library is another. But instead of trying to change the STL, it works with the current implementation. It provides a new allocator model, a [RawAllocator](#concept), several common [allocator implementations](#allocators) and [adapters](#adapters). Those adapters allow an easy integration with STL containers.


## <a name="installation"></a>Installation
There are two ways of using the library:

* [Biicode] \(easy):

    0\. Setup your project as biicode block, e.g. by running `bii buzz` in your project folder.

    1\. Include needed library header files via `<memory/*>`.

    2\. Specify the following include mapping in `biicode.conf` under `[includes]`: `memory/*: foonathan/memory/include/foonathan`. This will map all files of the form `<memory/*>` to the include folder of memory.

    3\. Run `bii find` to download all required files from the Biicode server automatically.

    4\. Run `bii build` to build this block. This library will be linked to it automatically.

    5\. Define [library options](#options) by running `bii configure -Doption=value` prior to building.

* CMake:

    0\. Setup your project to use CMake by adding an appropriate `CMakeLists.txt` file.

    1\. Save this library as a subdirectory of the project that is using it. You can use git submodules for example to make it easier.

    2\. Call `add_subdirectory()` with that folder to use it in your project.

    3\. Link the targets that are using it to `foonathan_memory`. It will automatically set up include paths and C++ standard.

    4\. Define [library options](#options) prior to calling `add_subdirectory()` by using `set(option value CACHE INTERNAL "" FORCE)` or use the command line.

Both ways require at least CMake version 3.1 and support the compilers listed [here](#compiler).


## <a name="concept"></a>RawAllocator concept
This library provides a new allocator concept, a *RawAllocator*. Where the classic *Allocator* works with types and similar to `new/delete`, a *RawAllocator* works with raw memory (hence the name) and is similar to `::operator new/::operator delete` or `malloc/free`. This allows it to be decoupled from concrete types and stops the need for templates (still, almost all allocators in this library are templates to allow maximum flexibility, but they are not required).
Another difference is the seperation between node and array allocations. A node is a single object, like an element of `std::list`. An array is a collection of such nodes. This is useful for memory pools, where there need to be a different approach depending on whether it is an array or node allocation. In addition, the *RawAllocator* support alignment requirement. The memory can be aligned to certain boundaries. This allows fine tuned allocation and efficiency.
The required interface for *RawAllocator* is as follows:
```cpp
// A raw allocator, only supports raw memory allocation.
// Similar to ::operator new/malloc. This avoids the need to be templated and to use one allocator for multiple types.
// The raw_allocator_traits can be specialized to adopt to another interface.
class raw_allocator
{
public:
    // Whether or not the allocator is stateful.
    // Non-stateful allocators don't need to be stored and can be default constructed on the fly.
    // Thus, it is probably an empty type.
    using is_stateful = std::true_type/std::false_type;

    // The allocator is required to be moveable.
    // Should not throw.
    raw_allocator(raw_allocator&&);
    raw_allocator& operator=(raw_allocator&&);

    // Allocates memory for a node. A node is a single object of given size and alignment.
    // Precondition: size <= max_node_size() && alignment <= max_alignment()
    void* allocate_node(std::size_t size, std::size_t alignment);

    // Allocates memory for an array of multiple nodes.
    // Precondition: count * size <= max_array_size() && alignment <= max_alignment()
    void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment);

    // Deallocates memory for a node. Must not throw.
    // Precondition: node must come from allocate_node with the same parameters and must not be null.
    void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept;

    // Deallocates memory for an array of nodes. Must not throw.
    // Precondition: array must come from allocate_array with the same paramters and must not be null.
    void deallocate_array(void *array, std::size_t count, std::size_t size, std::size_t alignment) noexcept;

    // Returns the maximum size of a node, inclusive. Should not throw.
    std::size_t max_node_size() const;

    // Returns the maximum size for an array (total size, no_elements * object_size), inclusive. Should not throw.
    std::size_t max_array_size() const;

    // Returns the maximum supported alignment, inclusive. Should not throw.
    std::size_t max_alignment() const;
};
```
Of course, there is a traits class `allocator_traits` for classes that do not support this interface directly - like STL-Allocators!

## <a name="allocators"></a>Allocators
There are currently the following classes that model *RawAllocator* or have specialized the traits in this library:

* `heap_allocator` - Allocates memory using `malloc`/`free`
* `new_allocator` - Allocates memory using ::operator new/::operator delete
* `memory_stack` - Allocates huge blocks of memory, then can be used in a stack manner, deallocation only via unwinding
* `memory_pool` - Allocates huge blocks of memory and seperates them into nodes of given size, great if you have multiple objects of the same size. Various policies to change pool type, such as `node_pool`, `array_pool`, `small_node_pool`.
* `memory_pool_collection` - Also known as bucket allocator. Maintains multiple pools at once to allow different sized allocation. Policies for pool type or the node sizes of the pools.
* `temporary_allocator`: A portable `alloca()` that uses a `thread_local` `memory_stack` instead of the native stack.

The `memory_*` allocators are special. They allocate a big block of memory and give it out one by one. If the block is exhausted, a new is one allocated. The block allocation can be controlled via a template parameter, they use a given implementation *RawAllocator* to allocate the big blocks (default is `default_allocator`).

## <a name="adapters"></a>Adapters
A new allocator model just by itself would be useless, because it can't be used with the existing model. For this case, there are adapters. The engine is the `allocator_reference`. This class stores a pointer to a *RawAllocator*. It allows copying of allocator classes as it is required by the STL containers. `raw_allocator_allocator` is a normal *Allocator* that stores one such `allocator_reference`. It forwards all allocation requests to it and thus to a user defined *RawAllocator*. Since the `get_allocator()` function returns a copy of the *Allocator* but `allocator_reference` stores a pointer, you can still access the original used allocator from a container. The new `propagate_on_XXX` members in `raw_allocator_allocator` have all been set to `std::true_type`. This ensure that an allocator always stays with its memory and allows fast moving. The `raw_allocator_allocator` thus allows that *RawAllocator* classes can be used with STL containers.

The new smart pointer classes don't use *Allocator* classes, they use *Deleter*. But there are also adapters for those and new `raw_allocate_unique/shared` function to easily create smart pointers whose memory is managed by *RawAllocator*.

There are also tracking adapters. A *Tracker* provides functions that are called on certain events, such as memory allocation or allocator growth (when they allocate new blocks from the implementation allocator). `tracked_allocator` takes a *RawAllocator* and a *Tracker* and combines them. This allows easily monitoring of memory usage. Due to the power of templates, `tracked_allocator` works with all classes modelling the concept of *RawAllocator* (or specialized the traits) including user defined ones.

Other adapters include a thread safe wrapper, that locks a mutex prior to accessing, or another ensuring a certain minimum alignment.

## <a name="compiler"></a>Compiler Support
[![Build status](https://ci.appveyor.com/api/projects/status/ef654yqyoqgvl472?svg=true)](https://ci.appveyor.com/project/foonathan/memory) [![Travis](https://img.shields.io/travis/joyent/node.svg)](#compiler)

Each release is automatically compiled and thus supported under the following compilers:

* GCC 4.7 to 4.9 on Linux
* clang 3.4 to 3.5 on Linux
* Visual Studio 12 on Windows

It requires many C++11 features, but there are compatibility options and replacement macros for `alignof`, `thread_local`, `constexpr` and `noexcept` and workarounds for missing `std::max_align_t` and `std::get_new_handler()`.

## <a name="options"></a>CMake Options
There are the following variables available to configure it:

* `FOONATHAN_MEMORY_DEFAULT_ALLOCATOR`: The default allocator used by the higher level allocator classes. Either `heap_allocator` or `new_allocator`. Default is `heap_allocator`.
* `FOONATHAN_MEMORY_THREAD_SAFE_REFERENCE`: Whether or not the `allocator_reference` is thread safe by default. Default is `ON`.
* `FOONATHAN_MEMORY_DEBUG_*`: specifies debugging options such as pointer check in `deallocate()` or filling newly allocated memory with values. All of them are enabled in `Debug` builds by default, the faster ones in `RelWithDebInfo` and none in `Release`.

* `FOONATHAN_HAS_*`: specifies compatibility options, that is, whether a certain C++ feature is available under your compiler. They are automatically detected, so there is usally no need to change them.
* `FOONATHAN_MEMORY_BUILD_*`: whether or not to build examples or tests. If this is `OFF` their CMake scripts are not even included. It is `ON` for standalone builds and `OFF` if used in `add_subdirectory()`. This option must not be changed on Biicode builds.
* `FOONATHAN_MEMORY_NAMESPACE_PREFIX`: If `ON`, everything is in namespace `foonathan::memory`. If `OFF`, you can use namespace `memory` directly (the default).
* `FOONATHAN_MEMORY_INCLUDE_PREFIX`: Similar to the namespace prefix, if `ON`, include directory is `<foonathan/memory/*>`, if `OFF`include directory is `<memory/*>` directly (the default). This option must not be changed on Biicode builds.

The following variables or targets are available if used with `add_subdirectory()`:

* `FOONATHAN_MEMORY_INCLUDE_DIR` (variable): The include directory for the header files.
* `FOONATHAN_MEMORY_VERSION_MAJOR/MINOR` (variable): Major and minor version of the library.
* `foonathan_memory` (target): The target of the library you can link to.
* `foonathan_memory_example_*` (target): The targets for the examples. Only available if `FOONATHAN_MEMORY_BUILD_EXAMPLES` is `ON`.
* `foonathan_memory_test` (target): The test target. Only available if `FOONATHAN_MEMORY_BUILD_TESTS` is `ON`.
* `foonathan_memory_profiling` (target): The profiling target. Only available if `FOONATHAN_MEMORY_BUILD_TESTS` is `ON`.

A list of all options with description is generated by calling `cmake -LH`.


[EASTL]:http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html
[Biicode]:http://www.biicode.com
