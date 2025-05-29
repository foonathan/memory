# Debugging options and error handling {#md_doc_debug_error}

## Error handling

By default, there are two exceptions thrown when something goes wrong.

The class [out_of_memory] is thrown when a low-level allocator like a [heap_allocator] runs out of memory.
It is inherited from `std::bad_alloc`.

The class [bad_allocation_size] is thrown when a higher-level allocator like a [memory_pool] is requested to allocate node bigger than its [node] size, for example.
Another case is an array which is simply too big. It is also inherited from `std::bad_alloc`.

This error handling mechanism can be configured to work without exceptions.
Each exception class contains a static `handler` function which can be set to a user-defined function.
This function gets called in the exception constructor.
It takes information about the allocator such as name and address and some exception-related information, such as the size for which the allocation fails.

The handler can do anything it wants, i.e. log the error.
If the handler returns, the exception will be thrown.

If exceptions are disabled in the library any `throw` statement will be translated to a call to `std::abort()`,
the handler functions are then especially useful as they will still be called.

## Debugging
<a name="debugging"></a>

There are also facilities useful for tracking memory related errors.

Memory leaks can be tracked down with the [leak_handler].
It will be called when an allocator is destroyed without all memory being deallocated.
There is a distinction between allocating memory through the [allocator_traits] or via the allocator specific interface directly,
ie. [memory_stack]'s `unwind()` function. Only (de-)allocating through the [allocator_traits] will be tracked,
since then the user deals with a generic allocator and cannot rely on proper deallocation as opposed to dealing with a specific allocator,
which will return all memory when it is destroyed.
Leak checking can be completely disabled by the CMake option `FOONATHAN_MEMORY_DEBUG_LEAK_CHECK`.

Invalid pointers on deallocation functions such as double-free or was-never-allocated can be tracked via the [invalid_pointer_handler].
While checking for was-never-allocated is relatively cheap, double-free can be a lot more expensive, especially for pool allocators,
where the whole pool has to be check if the pointer is already in the pool
(the pool is kept sorted in that case to reduce the time, but it makes deallocation still not constant as without check).
So both options can be disabled separately, by `FOONATHAN_MEMORY_DEBUG_POINTER_CHECK` and `FOONATHAN_MEMORY_DEBUG_DOUBLE_DEALLOC_CHECK` respectively.

The CMake option `FOONATHAN_MEMORY_DEBUG_FILL` controls pre-filling the memory.
This is useful for tracking errors coming from missing proper initialization after allocation or use-after-free.
The values can be read from the enum [debug_magic].

For catching overflow errors, the CMake option `FOONATHAN_MEMORY_DEBUG_FENCE_SIZE` can be set to an integral value.
It leads to additional fence memory being allocated before or after the memory.
Note that for alignment issues it may not be the exact size big.
On allocation the fence memory will be checked if it is still filled with the fence memory value,
if not the [buffer_overflow_handler] will be called.

Other internal assertions in the allocator code to test for bugs in the library can be controlled via the CMake option `FOONATHAN_MEMORY_DEBUG_ASSERT`.

[out_of_memory]: \ref foonathan::memory::out_of_memory
[bad_allocation_size]: \ref foonathan::memory::bad_allocation_size
[heap_allocator]: \ref foonathan::memory::heap_allocator
[memory_pool]: \ref foonathan::memory::memory_pool
[leak_handler]: \ref foonathan::memory::leak_handler
[buffer_overflow_handler]: \ref foonathan::memory::buffer_overflow_handler
[invalid_pointer_handler]: \ref foonathan::memory::invalid_pointer_handler
[allocator_traits]: \ref foonathan::memory::allocator_traits
[memory_stack]: \ref foonathan::memory::memory_stack
[debug_magic]: \ref foonathan::memory::debug_magic
