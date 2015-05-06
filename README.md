memory
======
The C++ STL allocator model has various flaws. For example, they are fixed to a certain type, because they are almost necessarily required to be templates. So you can't easily share a single allocator for multiple types. In addition, you can only get a copy from the containers and not the original allocator object. At least with C++11 they are allowed to be stateful and so can be made object not instance based. But still, the model has many flaws.
Over the course of the years many solutions have been proposed. for example EASTL[1]. This library is another. But instead of trying to change the STL, it works with the current implementation.

RawAllocator
------------
This library provides a new allocator concept, a *RawAllocator*. Where the classic *Allocator* works with types and similar to new/delete, a *RawAllocator* works with raw memory (hence the name) and is similar to ::operator new/::operator delete or malloc/free. This allows it to be decoupled from concrete types and stops the need for templates (still, almost all allocators in this library are templates to allow maximum flexibility, but they are not required).
Another difference is the seperation between node and array allocations. A node is a single object, like an element of std::list. An array is a collection of such nodes. This is useful for memory pools, where there need to be a different approach depending on whether it is an array or node allocation.
In addition, the *RawAllocator* support alignment requirement. The memory can be aligned to certain boundaries. This allows fine tuned allocation and efficiency. The required interface for *RawAllocator* is as follows:

    // A raw allocator, only supports raw memory allocation.
    // Similar to ::operator new/malloc. This avoids the need to be templated and to use one allocator for multiple types.
    // The raw_allocator_traits can be specialized to adopt to another interface.
    class raw_allocator
    {
    public:
        // Whether or not the allocator is stateful.
        // Non-stateful allocators don't need to be stored and can be default constructed on the fly.
        // The result of the getter function is always the same.
        using is_stateful = std::true_type/std::false_type;
         
        // The allocator is required to be moveable
        raw_allocator(raw_allocator&&);
        raw_allocator& operator=(raw_allocator&&);
        
        // Allocates memory for a node. A node is a single object of given size and alignment.
        // Precondition: size <= max_node_size() && alignment <= max_alignment()
        // Throws an exception derived from std::bad_alloc in case of failure.
        void* allocate_node(std::size_t size, std::size_t alignment);
        
        // Allocates memory for an array of multiple nodes.
        // Precondition: count * size <= max_array_size() && alignment <= max_alignment()
        // Throws an exception derived from std::bad_alloc in case of failure.
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment);
        
        // Deallocates memory for a node. Must not throw.
        void deallocate_node(void *node, std::size_t size, std::size_t alignment) noexcept;
        
        // Deallocates memory for an array of nodes. Must not throw.
        void deallocate_array(void *array, std::size_t count, std::size_t size, std::size_t alignment) noexcept;
        
        // Returns the maximum size of a node, inclusive. Should not throw.
        std::size_t max_node_size() const;
        
        // Returns the maximum size for an array (total size, no_elements * object_size), inclusive. Should not throw.
        std::size_t max_array_size() const;
        
        // Returns the maximum supported alignment, inclusive. Should not throw.
        std::size_t max_alignment() const;
    };
Of course, there is a traits class for classes that do not support this interface directly - like STL-Allocators!
There are currently the following classes that model *RawAllocator* or have specialized the traits in this library:
* heap_allocator - Allocates memory using malloc/free
* new_allocator - Allocates memory using ::operator new/::operator delete
* memory_stack - Allocates huge blocks of memory, then can be used in a stack manner, deallocation only via unwinding
* memory_pool - Allocates huge blocks of memory and seperates them into nodes of given size, great if you have multiple objects of the same size
* memory_pool_collection - Maintains multiple memory_pools at once to allow different sized allocation.

The last three are special allocators. They allocate a big block of memory and give it out one by one. If the block is exhausted, a new is one allocated. The block allocation can be controlled via a template parameter, they use a given implementation *RawAllocator* for it (default is heap_allocator).

Adapters
--------
A new allocator model just by itself would be useless, because it can't be used with the existing model. For this case, there are adapters. The engine is the raw_allocator_adapter. This class stores a pointer to a *RawAllocator*. It allows copying of allocator classes as it is required by the STL containers. raw_allocator_allocator is a normal *Allocator* that stores one such raw_allocator_adapter. It forwards all allocation requests to it and thus to a user defined *RawAllocator*. Since the get_allocator() function returns a copy of the *Allocator* but raw_allocator_adapter stores a pointer, you can still access the original used allocator from a container. The new propagate_on_XXX in raw_allocator_allocator members have all been set to std::true_type. This ensure that an allocator always stays with its memory and allows fast moving. The raw_allocator_allocator thus allows that *RawAllocator* classes can be used with STL containers.
The new smart pointer classes don't use *Allocator* classes, they use *Deleter*. But there are also adapters for those and new raw_allocate_unique/shared function to easily create smart pointers whose memory is managed by *RawAllocator*.
There are also tracking adapters. A *Tracker* provides functions that are called on certain events, such as memory allocation or allocator growth (when they allocate new blocks from the implementation allocator). tracked_allocator takes a *RawAllocator* and a *Tracker* and combines them. This allows easily monitoring of memory usage. Due to the power of templates, tracked_allocator works with all classes modelling the concept of *RawAllocator* including user defined ones.

Plans
-----
I have may plans for this library. The obvious one is providing more and fancier allocators. The first things I have in mind is a memory pool for small objects, the current design needs to store a pointer in each node. But also double_frame_allocators or similar types are planned as well as a portable replacement for alloca that uses a global, thread_local memory_stack and is useful for temporary short-term allocations with a similar speed than from the real stack.
But also more adapters are going to come. I need to put thread safety into the raw_allocator_allocator or write a different adapter for it. I also think about integrating the Boost.Pool[2] into it, if you don't trust or dislike my pool implementation.

[1] EASTL - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2271.html

[2] Boost.Pool - http://www.boost.org/doc/libs/1_57_0/libs/pool/doc/html/index.html
