0.3
---
* added debugging options such as memory filling and deallocation and leak check
* improved performance of pool allocators
* changed complete project structure and CMake
* many internal changes and bugfixes and automated testing

0.2
---
* added temporary_allocator as portable alloca
* added small_node_pool type optimized for low-overhead small object allocations
* added various allocator adapters including a thread_safe_allocator for locking
* better compiler support
* many internal changes and bugfixes

0.1-1
-----
* critical bugfix in memory_stack
* added smart pointer example

0.1
---
* first beta version
