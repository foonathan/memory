# Upcoming Changes

# 0.7-3

CMake improvements:

* Increase the CMake minimum required version to 3.14 to reflect reality. (#147)
* Fixes to container node size generation using CMake on MacOS. (#138)
* Don't regenerate container node sizes when they already exist.

Bugfixes:

* Workaround a compiler bug in MSVC. (#146)
* Fix broken multi-threading configuration. (#142)
* Check initial block size for `memory_pool_collection` properly. (#148)
* Fix bug in `memory_block_stack::owns()`. (#151)

# 0.7-2

Deprecate `virtual_memory_page_size`; use `get_virtual_memory_page_size()` instead. (#132)

CMake improvements:

* Generate container node sizes using CMake, which makes cross-compiling easier (#129)
* Set `CMAKE_RUNTIME_OUTPUT_DIRECTORY` properly to support shared libraries (#132)

# 0.7-1

Just bugfixes:

* CMake: automatically link libatomic on platforms where that is necessary (#95)
* catch small block sizes of `memory_pool` (#113)
* fix buffer overflow in `memory_pool_collection`'s array allocation (#99)
* fix compatibility with Windows UWP (#102)
* fix computation of `memory_pool::min_block_size` (#110)
* fix debug assertion in free lists (#111)

# 0.7

BREAKING: Removed the use of the compatibility library to automatically generate macros and workaround for older compilers.
The important compatibility workarounds like the `__builtin_clz` extension are still used, but workarounds for missing C++11 library features have been removed.
In particular, the library now requires compiler support for `noexcept`, `constexpr`, `alignof` and `thread_local`.
This means that GCC 4.8 and Visual Studio version 12.0 (both released in 2013), are now longer supported.

## Adapter

BREAKING: Remove `Mutex` support from `allocator_reference` and consequently from `std_allocator`, `allocator_deleter`, ...
Embedding the `Mutex` with the reference was *fundamentally* broken and unusable to ensure thread safety.
Use a reference to a `thread_safe_allocator` instead, which actually guarantees thread safety.

## Allocator

Add ability to query the minimal block size required by a `memory_pool` or `memory_stack` that should contain the given memory.
Due to internal data structures and debug fences this is more than the naive memory request, so it can be computed now.

## Bugfixes

* more CMake improvements for cross-compiling, among others
* bugfixes to support UWP (#80), VxWorks (#81) and QNX (#85, #88, among others)
* better support missing container node size (#59, #72, among others)
* fix alignment issues in debug mode
* fix tracking for allocators without block allocators

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
