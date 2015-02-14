#ifndef FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE
#define FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE

/// \file
/// \brief Utility base class generating functions.

#include <limits>

namespace foonathan { namespace memory
{
    /// \brief Base class that generates default implementations for the more repetitive functions.
    template <class Derived>
    class raw_allocator_base
    {
    public:
        raw_allocator_base() = default;
        
        raw_allocator_base(const raw_allocator_base&) = delete;
        raw_allocator_base(raw_allocator_base&&) = default;
        
        raw_allocator_base& operator=(const raw_allocator_base &) = delete;
        raw_allocator_base& operator=(raw_allocator_base &&) = default;
        
        /// @{
        /// \brief Array allocation forwards to node allocation.
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            return static_cast<Derived*>(this)->allocate_node(count * size, alignment);
        }
        
        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            static_cast<Derived*>(this)->deallocate_node(ptr, count * size, alignment);
        }
        /// @}
        
        /// @{
        /// \brief Returns maximum value.
        std::size_t max_node_size() const noexcept
        {
            return std::numeric_limits<std::size_t>::max();
        }
        
        std::size_t max_array_size() const noexcept
        {
            return std::numeric_limits<std::size_t>::max();
        }
        /// @}
        
    protected:
        ~raw_allocator_base() noexcept = default;    
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE
