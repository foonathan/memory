# Upcoming Changes

BREAKING: Removed the use of the compatibility library to automatically generate macros and workaround for older compilers.
The important compatibility workarounds like the `__builtin_clz` extension are still used, but workarounds for missing C++11 library features have been removed.
In particular, the library now requires compiler support for `noexcept`, `constexpr`, `alignof` and `thread_local`.
This means that GCC 4.8 and Visual Studio version 12.0 (both released in 2013), are now longer supported.

## Adapter

BREAKING: Remove `Mutex` support from `allocator_reference` and consequently from `std_allocator`, `allocator_deleter`, ...
Embedding the `Mutex` with the reference was *fundamentally* broken and unusable to ensure thread safety.
Use a reference to a `thread_safe_allocator` instead, which actually guarantees thread safety.

## Bugfixes

* more CMake improvements

---

# 0.6-2

Various bug fixes, compiler warning workarounds and CMake improvements accumulated over past two years.
Most notable changes:

* cross compilation works now
* `fallback_allocator` is default constructible if stateless
* add `unique_base_ptr` to support a unique ptr to a base class
* add `allocate_unique` overloads that take a custom mutex
* allocator deleters are default constructible

---

# 0.6-1

* fix CMake configuration error
* fix double free error in `segregator`
* add `static_assert()` when default constructing a stateful `std_allocator`
* fix various compiler warnings

# 0.6

## Tool

* better MSVC support
* improved compilation time

## Core

* add literal operators for memory sizes (`4_KiB`)
* more flexible `make_block_allocator`
* composable allocator concept

## Allocator

* improved `temporary_allocator`: explicit separation into `temporary_stack`, improved automatic creation
* new `memory_stack_raii_unwind` for automatic unwinding
* new `iteration_allocator`
* make allocators composable
* add facilities for joint memory allocations

## Adapter

* add `shared_ptr_node_size`
* add `string` container typedef
* add `fallback_allocator`
* add `segregator`

## Bugfixes

* OSX support
* various warnings fixed

---

# 0.5
* improved CMake build system, now supports cmake installation and `find_package()`
* improved low-level allocators and added `malloc_allocator`
* add virtual memory interface and allocators
* add allocators using a fixed-sized storage block
* introduced `BlockAllocator` concept and various implementations
* new class template `memory_arena` that is used inside the higher level allocators, allows more control over the internal allocations
* add wrappers/adapters for the polymorphic memory resource TS
* improved tracking classes
* other improvements like concept checks and more exception classes
* internal changes

---

# 0.4

* polished up the interface, many breaking changes in the form of renaming and new header files
* added unified error handling facilities and handler functions in case exceptions are not supported
* improved old allocator adapters by introducing allocator_storage template
* improved allocator_traits making them more powerful and able to handle Allcoator types directly
* added type-erased allocator storage
* added node-size debugger that obtains information about the container node sizes
* most parts now work on a freestanding implementation
* used foonathan/compatibility for CMake compatibility checks
* added miscellaneous tiny features all over the place
* many internal changes and bugfixes

---

# 0.3

* added debugging options such as memory filling and deallocation and leak check
* improved performance of pool allocators
* changed complete project structure and CMake
* many internal changes and bugfixes and automated testing

---

# 0.2

* added temporary_allocator as portable alloca
* added small_node_pool type optimized for low-overhead small object allocations
* added various allocator adapters including a thread_safe_allocator for locking
* better compiler support
* many internal changes and bugfixes

---

# 0.1-1

* critical bugfix in memory_stack
* added smart pointer example

---

# 0.1

* first beta version
