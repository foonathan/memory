# Allocator adapters and storage classes {#md_doc_adapters_storage}

In addition to the various allocator classes, it also provides a couple of adapters and storage classes for [RawAllocator] instances.

## Allocator storage classes

Allocator storage classes are classes that store allocator objects.
They are all aliases for the class template [allocator_storage] with a certain [StoragePolicy].

The class simply delegates to the policy and provides the same constructors, but also the full set of [RawAllocator] member functions.
This allows accessing an allocator stored in an [allocator_storage] directly without using the traits.
A `Mutex` can also be specified that takes care of synchronization, the member function `lock()` returns a synchronized proxy object that acts like a pointer.

In addition to the following predefined policy classes it is possible to define your own.
Everything is defined in the header file allocator_storage.hpp.

### Direct allocator storage

The [StoragePolicy] [direct_storage] stores an allocator object directly as member.
It can be initialized by moving in another [RawAllcoator] instance.
Moving it also moves the allocator.

The alias [allocator_adapter] is an [allocator_storage] with this policy and no mutex.
It simply provides the full interface for the allocator without any additional semantics.
A little bit more semantics provides the alias [thread_safe_allocator] it synchronizes access through a specified mutex.

### Reference allocator storage

The [StoragePolicy] [reference_storage] stores a pointer to an allocator object.
Although it stores a pointer, it always references an object, i.e. it is never `null`.

It provides three slightly different semantics depending on whether or not the allocator is stateful:

* For stateful allocator, it takes a reference to it. Then it will store a pointer to the given allocator.
It does not take ownership, i.e. the passed allocator object must live longer than the reference to it!

* For stateless allocators, it uses a `static` object in order to return a reference in `get_allocator()`.
But this means that they don't actually depend on the lifetime of the given allocator and also can take temporaries.

* For special allocators that already provide reference semantics (determined through traits specialization), it behaves like a [direct_storage] policy.

In either case, the class is nothrow copyable and never actually moves the referred allocator, just copies the pointer.
A copy of a [reference_storage] references the same allocator as the origin.

The alias [allocator_reference] uses this storage policy.

### Type-erased reference allocator storage

The [reference_storage] takes the type of the allocator being stored.
A specialization takes the tag type [any_allocator].
It provides type-erased semantics.

The constructors are templated to take any [RawAllocator] - with the same restrictions for statefulness as in the normal case -
and stores a pointer to it using type-erasure.

The accessor functions return the base class used in the type-erasure which provides the full [RawAllocator] members.
Note that it is not possible to get back to the actual type, i.e. call functions in the actual allocator interface.

The tag type can be used anywhere where an [allocator_reference] is used, i.e. [allocator_deleter], [std_allocator] or custom containers.
For convienience, the alias [any_reference_storage] simply refers to this specialization, as does the actual storage class [any_allocator_reference].

## std_allocator

The class [std_allocator] takes a [RawAllocator], stores it in an [allocator_reference] and provides the `Allocator` interface.
This allows using raw allocator objects with classes requiring the standardized concept like STL containers.
It takes care of allocator propagation (always propagate), comparision and provides the full boilerplate.

The tag type [any_allocator] can be used to enable type erasure, the alias [any_std_allocator] is exactly that.

They are defined in the header std_allocator.hpp.

## Tracking

A special case of adapter is the class [tracked_allocator] defined in the header tracking.hpp.
It allows to track the called functions on a [RawAllocator].
This is done via a [Tracker].

A `Tracker` provides functions that gets called when the corresponding function on the allocator gets called.
For example, the function `allocate_node()` leads to call on the tracker function `on_allocate_node()`.

This is an example of a [memory_pool] that has been tracked with a `Tracker` that logs all (de-)allocations:

```cpp
#include <iostream>

#include <memory/memory_pool.hpp> // for memory_pool
#include <memory/tracking.hpp> // for tracked_allocator

struct log_tracker
{
    void on_node_allocation(void *mem, std::size_t size, std::size_t) noexcept
    {
        std::clog << this << " node allocated: ";
        std::clog << mem << " (" << size << ") " << '\n';
    }

    void on_array_allocation(void *mem, std::size_t count, std::size_t size, std::size_t) noexcept
    {
        std::clog << this << " array allocated: ";
        std::clog << mem << " (" << count << " * " << size << ") " << '\n';
    }

    void on_node_deallocation(void *ptr, std::size_t, std::size_t) noexcept
    {
        std::clog << this << " node deallocated: " << ptr << " \n";
    }

    void on_array_deallocation(void *ptr, std::size_t, std::size_t, std::size_t) noexcept
    {
        std::clog << this << " array deallocated: " << ptr << " \n";
    }
};

int main()
{
    auto tracked_pool = memory::make_tracked_allocator(log_tracker{}, memory::memory_pool<>(16, 1024));
    // go on using the tracked_pool
}
```

The `log_tracker` above uses the address of the tracker object to identify a certain allocator in the output,
this is completely legal.
Note that the function `make_tracked_allocator()` which returns the appropriate [tracked_allocator] takes ownership over the pool,
you can either pass a temporary as shown or move in an existing pool.
The result `tracked_pool` provides the full [RawAllocator] interface and can be used as usual,
except that all (de-)allocations are logged.

## Other adapters

### aligned_allocator

The allocator adapter [aligned_allocator] wraps a [RawAllocator] and ensure a certain minimum alignment on all functions.

[any_allocator]: \ref foonathan::memory::any_allocator
[allocator_storage]: \ref foonathan::memory::allocator_storage
[allocator_deleter]: \ref foonathan::memory::allocator_deleter
[allocator_adapter]: \ref foonathan::memory::allocator_adapter
[allocator_reference]: \ref foonathan::memory::allocator_reference
[any_allocator_reference]: \ref foonathan::memory::any_allocator_reference
[thread_safe_allocator]: \ref foonathan::memory::thread_safe_allocator
[direct_storage]: \ref foonathan::memory::direct_storage
[reference_storage]: \ref foonathan::memory::reference_storage
[any_reference_storage]: \ref foonathan::memory::any_reference_storage
[std_allocator]: \ref foonathan::memory::std_allocator
[any_std_allocator]: \ref foonathan::memory::any_std_allocator
[aligned_allocator]: \ref foonathan::memory::aligned_allocator
[memory_pool]: \ref foonathan::memory::memory_pool
[RawAllocator]: md_doc_concepts.html#concept_rawallocator
[StoragePolicy]: md_doc_concepts.html#concept_storagepolicy
[Tracker]: md_doc_concepts.html#concept_tracker
