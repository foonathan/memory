# Upcoming

## Tool

* better MSVC support
* improved compilation time

## Core

* add literal operators for memory sizes (`4_KiB`)
* more flexible `make_block_allocator`

## Allocator

* improved `temporary_allocator`: explicit seperation into `temporary_stack`, improved automatic creation
* new `memory_stack_raii_unwind` for automatic unwinding
* new `iteration_allocator`

## Adapter

* add `shared_ptr_node_size`

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
