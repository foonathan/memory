# Writing own RawAllocator classes {#md_doc_writing_allocators}

There are, in general, three different ways and one special case to write a [RawAllocator] class.
See the link for the exact requirements and behavior for each function.

## 0. Write a normal Allocator class

Just go ahead and write a normal `Allocator` class. It will work just fine and can be used anywhere a [RawAllocator] is required.
Keep in mind, though, that the `construct` or `destroy` functions will not be called and its pointer typedefs not used.

## 1. Fulfill the requirements for the default allocator_traits

This is the easiest way. The default specialization of [allocator_traits] will forward to member functions, if they exist,
and has some fallback, if they don't.
The following class overrides all the fallbacks:

```cpp
struct raw_allocator
{
    using is_stateful = std::integral_constant<bool, Value>;

    void* allocate_node(std::size_t size, std::size_t alignment);
    void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept;

    void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment);
    void deallocate_array(void *ptr, std::size_t count, std::size_t size, std::size_t alignment) noexcept;

    std::size_t max_node_size() const;
    std::size_t max_array_size() const;
    std::size_t max_alignment() const;
};
```

There are fallbacks for every function except `allocate_node()` and `deallocate_node()`.
A minimum class thus only needs to provide those two functions.
The fallbacks "do the right thing", for example `allocate_array()` forwards to `allocate_node()`, `is_stateful` is determined via `std::is_empty`
and `max_node_size()` returns the maximum possible value.

Keep in mind that a [RawAllocator] has to be nothrow moveable and be valid to be used as a non-polymorphic base class,
i.e. as a `private` base to use EBO.

The full interface is provided by the [allocator_storage] typedefs.
Other classes where this approach is used are [heap_allocator] or [aligned_allocator].
The latter also provides the full interface.

## 2. Specialize the allocator_traits

But sometimes it is not attractive to provide the full interface.
An example is the library class [memory_stack].
Its interface consists of typical behaviors required for a stack, like unwinding,
and it does not make sense to provide a `deallocate_node()` function for it since there is no direct way to do so - only via unwinding.

In this case, the [allocator_traits] can be specialized for your type.
Keep in mind that it is in the sub-namespace `memory` of the namespace `foonathan`.
It needs to provide the following interface:

```cpp
template <>
class allocator_traits<raw_allocator>
{
public:
    using allocator_type = raw_allocator;
    using is_stateful = std::integral_constant<bool, Value>;

    static void* allocate_node(allocator_type &state, std::size_t size, std::size_t alignment);
    static void deallocate_node(allocator_type &state, void *node, std::size_t size, std::size_t alignment) noexcept;

    static void* allocate_array(allocator_type &state, std::size_t count, std::size_t size, std::size_t alignment);
    static void deallocate_array(allocator_type &state, void *array, std::size_t count, std::size_t size, std::size_t alignment) noexcept;

    static std::size_t max_node_size(const allocator_type &state);
    static std::size_t max_array_size(const allocator_type &state);
    static std::size_t max_alignment(const allocator_type &state);
};
```

This approach is used in the mentioned [memory_stack] but also the [memory_pool] classes.

## 3. Forward all behavior to another class

The [allocator_traits] provide a typedef `allocator_type`.
This type is the actual type used for the (de-)allocation and will be stored in all classes taking a [RawAllocator].
Its only requirement is that it is implicitly constructible from the actual type instantiated and that it is a [RawAllocator].

The main use for this typedef is to support `Allocator` classes.
They need to be rebound to `char` to allow byte-size allocations prior before they are actually used.

Using this technique otherwise is rather esoteric and I do not see any reason for it, but it is possible.
Let there be a class `raw_allocator` that is a [RawAllocator], i.e. it provides the appropriate traits interface using any of the mentioned ways.
This class also provides a constructor taking the class `my_allocator` that wants to forward to it.
Then you only need to write:

```cpp
class my_allocator {...};

class raw_allocator
{
public:
    raw_allocator(my_allocator &)
    {
        ...
    }

    // provides the required interface or has a traits specialization
};

...
template <>
class allocator_traits<my_allocator>
: public allocator_traits<raw_allocator>
{};
```

[allocator_traits]: \ref foonathan::memory::allocator_traits
[allocator_storage]: \ref foonathan::memory::allocator_storage
[aligned_allocator]: \ref foonathan::memory::aligned_allocator
[heap_allocator]: \ref foonathan::memory::heap_allocator
[memory_stack]: \ref foonathan::memory::memory_stack
[memory_pool]: \ref foonathan::memory::memory_pool
[RawAllocator]: md_doc_concepts.html#concept_rawallocator
