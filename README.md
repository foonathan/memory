memory
======
This library provides various memory allocators for high-performance allocation and deallocation. These allocators are provided in the form of a new allocator concept: RawAllocator. A RawAllocator is an improved version over the classical STL-Allocator. There are various wrapper classes and traits to convert between the two types. Each RawAllocator has the following interface or an appropriate specialization of the raw_allocator_traits:

    // A raw allocator, only supports raw memory allocation.
    // Similar to ::operator new/malloc. This avoids the need to be templated and to use one allocator for multiple types.
    // The raw_allocator_traits can be specialized to adopt to another interface.
    class raw_allocator
    {
    public:
        // Whether or not the allocator is stateful.
        // Non-stateful allocators don't need to be stored and can be default constructed on the fly.
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
