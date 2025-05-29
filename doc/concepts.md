# Concepts and overall requirements {#md_doc_concepts}

## Node
<a name="concept_node"></a>

A *node* is the region of storage needed to hold a single object.
This storage region is identified via a pointer which is the *address* of the node.
The terms *node* and *address of the node* are used interchangeable.

It can be described using two properties: a *size* and an *alignment*.
Both are represented as values of type `std::size_t`.
The *alignment* must be a non-negative power of two.
It describes the alignment requirement of the storage region,
i.e. the address is dividable by the alignment.
The *size* must be any valid value of `std::size_t` except `0`.
It describes the size of the storage region,
`size` bytes after the address are available for the node.

The following requirements must be fulfilled to create an object of type `T` in a node,
i.e. to call a placement new `new(address) T(ctor-args)`:

* The alignment of the node must be at least as big as the alignment of the type, returned by `alignof(T)`.
If it is bigger, the type is *over-aligned* in this node, otherwise, it is *normal-aligned*.

* The size of the node must be at least as big as the size of the type, returned by `sizeof(T)`.

*Example:* A node returned by a call to `allocate_node(sizeof(T), alignof(T))` of a [RawAllocator](#concept_rawallocator)
always fulfills these requirements for the type `T`.

## Array of nodes
<a name="concept_array"></a>

An *array of nodes* is a sequence of nodes whose storage regions are consecutively in memory.
The *address* of the array is simply the address of the first node in the array.
The terms *array* and *address of the array* are used interchangeable.

In addition to the size and alignment, it has an additional property, the *count*.
The *count* is the number of nodes and must be a valid value of type `std::size_t` except `0`.
The *size* is the size of each node and has the same requirements as for a node.
The *alignment* is the alignment of the first node in the array and has the same requirements as for a node.

The `i`th node of the array is at position `adress + i * size`.
The total memory occupied by an array is thus `count * size`.
The size specifies the alignment of each node except the first implicit through the position.

The following requirements must be fulfilled to create an array of `n` `T` objects in an array of nodes,
i.e. to create an object in each node:

* The count of nodes must be at least `n`.

* The alignment of the array must be at least `alignof(T)`.

* The size of each node must be at least `sizeof(T)` and a multiple of `alignof(T)`.
This is required to ensure proper alignment of each node in the array.
*Note:* To create over-aligned types in the nodes, the alignment must be the stricter alignment
and the size must be a multiple of the stricter alignment.

*Example:* An array of nodes returned by a call to `allocate_array(n, sizeof(T), alignof(T))` of a [RawAllocator](#concept_rawallocator)
always fulfills these requirements for the type `T`.
A call of the form `allocate_array(n, size, align)` where `align` is a stricter alignment
and `size` is a multiple of `align` bigger than `sizeof(T)` returns an array of nodes
where each node is over-aligned.

## RawAllocator
<a name="concept_rawallocator"></a>

A `RawAllocator` is the new type of allocator used in this library.
Unlike the `Allocator` it does not work on a certain type directly,
but only in terms of nodes and arrays of nodes.
Thus it is unable to specify things like pointer types or construction function.
It is only responsible for allocating and deallocating memory for nodes.

A `RawAllocator` can either be *stateful* or *stateless*.
A stateful allocator has some state, i.e. member variables, that need to be stored across calls.
A stateless allocator can be constructed on-the-fly for each member function call.
A pointer allocated by one instance of a stateless allocator can be deallocated with any other instance.
All member functions are assumed to be thread-safe and can be called without synchronization.
This is not valid for stateful allocator.
Most of the time stateless allocators are also empty types, although this is not required
(*note:* it does not make much sense for them to be not-empty, since the values of the member variables is not required to be the same.)
An additional requirement for stateless allocator is that they have a default constructor.

Access to a `RawAllocator` is only done via the class [allocator_traits].
It can be specialized for own `RawAllocator` types.
The requirements for such a specialization are shown in the following table,
where `traits` is `allocator_traits<RawAllocator`, `alloc` is an instance of type `traits::allocator_type`,
`calloc` is a `const` `alloc`, `size` is a valid node size, `alignment` is a valid alignment, `count` is a valid array count,
`node` is a [node](#concept_node) returned by `traits::allocate_node` and `array` is an [array](#concept_array) returned by `traits::allocate_array`:

Expression|Return Type|Throws|Description
----------|-----------|------|-----------
`traits::allocator_type` | `RawAllocator` (most of the time) | - (typedef) | *Note*: The default specialization uses this typedef to rebind a standard `Allocator` to `char`, to be able to use it to allocate single bytes. In most other cases, it should be the same type as the template parameter, if not, it must be implicitly convertible. Be aware that this is the type actual being stored and passed to all other functions.
`traits::is_stateful` | `std::true_type` or `std::false_type` or inherited | - (typedef) | Describes whether or not an allocator is stateful.
`traits::allocate_node(alloc, size, alignment)` | `void*` | `std::bad_alloc` or derived | Allocates a [node](#concept_node) and returns its address. Must not return `nullptr`.
`traits::allocate_array(alloc, count, size, alignment)` | `void*` | `std::bad_alloc` or derived | Allocates an [array](#concept_array) and returns its address. Must not return `nullptr`.
`traits::deallocate_node(alloc, node, size, alignment)` | `void` | must not throw | Deallocates a [node](#concept_node). `alloc`, `size` and `alignment` must be the same as in the allocation.
`traits::deallocate_array(alloc, array, count, size, alignment)` | `void` | must not throw | Deallocates an [array](#concept_array). `alloc`, `count`, `size` and `alignment` must be the same as in the allocation.
`traits::max_node_size(calloc)` | `std::size_t` | can throw anything, but should throw nothing | Returns the maximum size for a [node](#concept_node), i.e. the maximum value allowed as `size`. *Note:* Only an upper-bound value, actual maximum might be less.
`traits::max_array_size(calloc)` | `std::size_t` | can throw anything, but should throw nothing | Returns the maximum *raw* size for an [array](#concept_array), i.e. the maximum value allowed for `count * size`. *Note:* Only an upper-bound value, actual maximum might be less.
`traits::max_alignment(calloc)` | `std::size_t` | can throw anything, but should throw nothing | Returns the maximum supported alignment, i.e. the maximum value allowed for `alignment`. Must be at least `alignof(std::max_align_t)`.

The typedef `traits::allocator_type` is the actual *state* type of the allocator.
This is the type being stored and passed to all functions.
It must be implicitly convertible from a `RawAllocator`, provide move operations that do not throw
and must be a valid base class, i.e. not a built-in type or marked `final`, but does not need virtual functions.

The two allocation functions must never return a `nullptr`. If the allocation was unsuccessful,
they can either throw an exception derived from `std::bad_alloc` or terminate the program (not recommended).
They must be prepared to handle sizes or alignments bigger than the values returned by `max_*`, i.e. by throwing an exception.

Moving a stateful `RawAllocator` moves the ownership over the allocated memory, too.
That means that after a move, memory allocated by the old allocator must be freed by the new one,
not by the old one.
But a moved from allocator must still be usable for further memory allocations.
For stateless allocators this is not required, since all objects must deallocate all memory allocated by any other object.

To allow for an easier use, the default specialization of the [allocator_traits]
forwards to the appropriate member functions or uses the specified fallback:

Expression|RawAllocator|Fallback
----------|------------|--------
`traits::allocator_type` | `RawAllocator` | see below
`traits::is_stateful` | `RawAllocator::is_stateful` | empty types will be stateless and non-empty types stateful
`traits::allocate_node(alloc, size, alignment)` | `alloc.allocate_node(size, alignment)` | see below
`traits::allocate_array(alloc, count, size, alignment)` | `alloc.allocate_array(count, size, alignment)` | `traits::allocate_node(alloc, count * size, alignment)`
`traits::deallocate_node(alloc, node, size, alignment)` | `alloc.deallocate_node(node, size, alignment)` | see below
`traits::deallocate_array(alloc, array, count, size, alignment)` | `alloc.allocate_array(array, count, size, alignment)` | `traits::deallocate_node(alloc, count * size, alignment)`
`traits::max_node_size(calloc)` | `calloc.max_node_size()` | maximum value of type `std::size_t`
`traits::max_array_size(calloc)` | `calloc.max_array_size()` | `traits::max_node_size(calloc)`
`traits::max_alignment(calloc)` | `calloc.max_alignment()` | `alignof(std::max_align_t)`

To allow rebinding required for traditional `Allocator`s, there is an additional behavior when selecting the fallback.
If the parameter of the `allocator_traits` contains a typedef `value_type`, `traits::allocator_type` will rebind the type to `char`.
This is done in the same way `std::allocator_traits` does it, i.e. first try to access the `rebind` member struct,
then a type `alloc<T, Args...>` will be rebound to `alloc<char, Args...>`.
If the parameter does not provide a member function `allocate_node`, it will try and call the allocation function required by the `Allocator` concept,
i.e. `static_cast<void*>(alloc.allocate(size)`, likewise for `deallocate_node` which will call forward to the deallocation function `alloc.deallocate(static_cast<char*>(node), size)`.

This enables the usage of any type modelling the `Allocator` concept where a `RawAllocator` is expected.
It is only enabled, however, if the `Allocator` does not provide custom `construct()`/`destroy()` function since they would never be called.
The checking can be overriden by specializing the traits class [allocator_is_raw_allocator](\ref foonathan::memory::allocator_is_raw_allocator).
Note that it does *not* use the `std::allocator_traits` but calls the functions directly enabling only the `Allocator` classes that do not have specialized the traits template.

For exposition, this is the minimum required interface for a `RawAllocator` without an appropriate specialization:

```cpp
struct min_raw_allocator
{
    min_raw_allocator(min_raw_allocator&&) noexcept;
    ~min_raw_allocator() noexcept;
    min_raw_allocator& operator=(min_raw_allocator&&) noexcept;

    void* allocate_node(std::size_t size, std::size_t alignment);
    void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept;
};
```

*Note*: If a RawAllocator provides a member function for allocation/deallocation, it is not allowed to mix those two interfaces,
i.e. allocate memory through the traits and deallocate through the member function or vice-versa.
It is completely allowed that those functions do completely different things.

### Composable RawAllocator
<a name="concept_composableallocator"></a>

A RawAllocator can be *composable*.
Access to the composable (de)allocation functions is only done through the [composable_allocator_traits].
It can be specialized for your own allocator types.
The requirements for such a specialization are shown in the following table,
where `ctraits` is `composable_allocator_traits<RawAllocator`, `alloc` is an instance of type `traits::allocator_type`, `size` is a valid node size, `alignment` is a valid alignment, `count` is a valid array count,
`node` is any non-null [node](#concept_node) and `array` is any non-null [array](#concept_array):

Expression | Return Type | Description
-----------|-------------|------------
`ctraits::allocator_type` | `allocator_traits<RawAllocator>::allocator_type` | just forwards to the regular traits
`ctraits::try_allocate_node(alloc, size, alignment)` | `void*` | Similar to the `allocate_node()` function but returns `nullptr` on failure instead of throwing an exception.
`ctraits::try_allocate_array(alloc, count, size, alignment)` | `void*` | Similar to the `allocate_array()` function but returns `nullptr` on failure instead of throwing an exception.
`ctraits::try_deallocate_node(alloc, node, size, alignment)` | `bool` | Similar to the `deallocate_node()` function but can be called with *any* [node](#concept_node). If that node was allocated by `alloc`, it will be deallocated and the function returns `true`. Otherwise the function has no effect and returns `false`.
`ctraits::try_deallocate_array(alloc, array, count, size, alignment)` | `bool` | Similar to the `deallocate_array()` function but can be called with *any* [array](#concept_array). If that array was allocated by `alloc`, it will be deallocated and the function returns `true`. Otherwise the function has no effect and returns `false`.

Unlike the normal allocation functions, the composable allocation functions are allowed to return `nullptr` on failure,
they must never throw an exception.
The deallocation function can be called with arbitrary nodes/arrays.
The allocator must be able to detect whether they were originally allocated by the allocator and only deallocate them if that is the case.
You are not allowed to mix the composable and normal allocation functions.

Like [allocator_traits] the default [composable_allocator_traits] specialization forwards to member functions or uses a fallback:


Expression|RawAllocator|Fallback
----------|------------|--------
`ctraits::allocator_type` | - | `allocator_traits<RawAllocator>::allocator_type`
`ctraits::try_allocate_node(alloc, size, alignment)` | `alloc.try_allocate_node(size, alignment)` | none, required
`ctraits::try_allocate_array(alloc, count, size, alignment)` | `alloc.try_allocate_array(count, size, alignment)` | `ctraits::try_allocate_node(alloc, count * size, alignment)`
`ctraits::try_deallocate_node(alloc, node, size, alignment)` | `alloc.try_deallocate_node(node, size, alignment)` | non, required
`ctraits::try_deallocate_array(alloc, array, count, size, alignment)` | `alloc.try_deallocate_array(array, count, size, alignment)` | `ctraits::try_deallocate_node(alloc, array, count * size, alignment)`

## BlockAllocator
<a name="concept_blockallocator"></a>

Some allocator types manage huge memory blocks and returns part of them in their allocation functions.
Such huge memory blocks are managed by a memory arena, implemented in the class [memory_arena].

The size and the allocation of the memory blocks is controlled by a `BlockAllocator`.
It is responsible to allocate and deallocate those blocks. It must be nothrow moveable and a valid base class, i.e. not `final`. In addition, it must provide the following:

Expression|Semantics
----------|---------
`BlockAllocator(block_size, args)`|Creates a `BlockAllocator` by giving it a non-zero initial block size and optionally multiple further arguments.
`alloc.allocate_block()`|Returns a new [memory_block] object that is the next memory block.
`alloc.deallocate_block(block)`|Deallocates a `memory_block`. Deallocation will be done in reverse order.
`calloc.next_block_size()`|Returns the size of the `memory_block` in the next allocation.

The alignments of the allocated memory blocks must be the maximum alignment.

This is a sample `BlockAllocator` that uses `new` for the allocation:

```cpp
class block_allocator
{
public:
    block_allocator(std::size_t block_size)
    : block_size_(block_size) {}
    
    memory_block allocate_block()
    {
        auto mem = ::operator new(block_size_);
        return {mem, block_size_};
    }
    
    void deallocate_block(memory_block b)
    {
        ::operator delete(b.memory);
    }
    
    std::size_t next_block_size() const
    {
        return block_size_;    
    }
    
private:
    std::size_t block_size_;    
};
```

## StoragePolicy
<a name="concept_storagepolicy"></a>

A `StoragePolicy` stores a [RawAllocator](#concept_rawallocator) and is used with the class template [allocator_storage].
It specifies how the allocator is stored, i.e. whether it is stored directly or only a pointer to it.
It must always store a `RawAllocator` instance.
The `StoragePolicy` must be class that is nothrow moveable and be a valid base class, i.e. not `final`.

In addition it must provide the following:

Expression|Semantics
----------|---------
`StoragePolicy::allocator_type` | The type of the allocator being stored as determinted through the [allocator_traits]. For a type-erased storage, it can be the type-erased base class.
`StoragePolicy(args)` | Creates the `StoragePolicy`. `args` can be anything. It is used to create the allocator.
`policy.get_allocator()` | Returns a reference to the `allocator_type`. Must not throw. May return a `const` reference, if `policy` is `const`.
`policy.is_composable()` | Returns whether or not the `allocator_type` is a [ComposableAllocator](#concept_composableallocator)

For exposition, this is a sample `StoragePolicy`.
Note that it is not required to be a template, although it does not make much sense otherwise.

```cpp
template <class RawAllocator>
class storage_policy
{
public:
    using allocator_type = typename memory::allocator_traits<RawAllocator>::allocator_type;

    storage_policy(RawAllocator &&alloc) noexcept
    : alloc_(std::move(alloc)) {}

    allocator_type& get_allocator() noexcept
    {
        return alloc_;
    }

    const allocator_type& get_allocator() const noexcept
    {
        return alloc_;
    }
    
    bool is_composable() const noexcept
    {
        return memory::is_composable_allocator<allocator_type>::value;
    }

private:
    allocator_type alloc_;
};
```

## Segregatable
<a name="concept_segregatable"></a>

A `Segregatable` stores a [RawAllocator](#concept_rawallocator) and controls for which allocations it will be used.
It is used in [binary_segregator].

It must be nothrow movable and provide the following:

Expression|Type|Semantics
----------|----|---------
`Segregatable::allocator_type`|some [RawAllocator](#concept_rawallocator)|The type of the allocator it controls.
`segregatable.get_allocator()`|`allocator_type&`|A reference to the allocator object it controls.
`const_segregatable.get_allocator()`|`const allocator_type&`|A `const` reference to the allocator object it controls.
`segregatable.use_allocate_node(size, alignment)`|`bool`|Whether or not the allocator object will be used for a node allocation with this specific properties. If it returns `true`, `allocate_node()` of the allocator object with the same parameters will be called, if it returns `false`, it will not be used.
`segregatable.use_allocate_array(count, size, alignment)`|`bool`|Whether or not the allocator object will be used for an array allocation with this specific properties. If it returns `true`, `allocate_array()` of the allocator object with the same parameters will be called, if it returns `false`, it will not be used.

For exposition, this is a simple `Segregatable` that will always use the given allocator:

```cpp
template <class RawAllocator>
class segregatable
{
public:
    using allocator_type = typename memory::allocator_traits<RawAllocator>::allocator_type;

    segregatable(RawAllocator &&alloc) noexcept
    : alloc_(std::move(alloc)) {}

    allocator_type& get_allocator() noexcept
    {
        return alloc_;
    }

    const allocator_type& get_allocator() const noexcept
    {
        return alloc_;
    }
    
    bool use_allocate_node(std::size_t, std::size_t) noexcept
    {
        return true;
    }
    
    bool use_allocate_array(std::size_t, std::size_t) noexcept
    {
        return true;
    }

private:
    allocator_type alloc_;
};
```

## Tracker
<a name="concept_tracker"></a>

A `Tracker` tracks allocation and/or deallocation of a `RawAllocator` and is used in the class template [tracked_allocator].
It is a moveable class that can be used as base class.
No operation on a `Tracker` may throw.
The address of a `Tracker` can be used as a unique, runtime identifier for a certain `RawAllocator`.

An instance `tracker` of it must provide the following functions:

Expression|Semantics
----------|---------
`tracker.on_node_allocation(node, size, alignment)` | Gets called after a [node](#concept_node) with given properties has been allocated.
`tracker.on_node_deallocation(node, size, alignment)` | Gets called before a [node](#concept_node) with given properties is deallocated.
`tracker.on_array_allocation(array, count, size, alignment)` | Same as the [node](#concept_node) version, but for [arrays](#concept_array).
`tracker.on_array_deallocation(array, count, size, alignment)` | Same the [node](#concept_node) version, but for [arrays](#concept_array).

*Note*: Those tracking functions are also called after a succesful composable (de)allocation function.

A *deep tracker* also tracks a [BlockAllocator](#concept_blockallocator) of another allocator
and thus allows monitoring the often more expensive big allocations done by it.
Such a `Tracker` must provide the following additional functions:

Expression|Semantics
----------|---------
`tracker.on_allocator_growth(memory, size)` | Gets called after the block allocator has allocated the passed memory block of given size.
`tracker.on_allocator_shrinkage(memory, size)` | Gets called before a given memory block of the block allocator will be deallocated.

For exposition, this is a sample `Tracker`:

```cpp
struct tracker
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
```

[allocator_storage]: \ref foonathan::memory::allocator_storage
[allocator_traits]: \ref foonathan::memory::allocator_traits
[composable_allocator_traits]: \ref foonathan::memory::composable_allocator_traits
[memory_arena]: \ref foonathan::memory::memory_arena
[memory_block]: \ref foonathan::memory::memory_block
[binary_segregator]: \ref foonathan::memory::binary_segregator
[tracked_allocator]: \ref foonathan::memory::tracked_allocator
