#ifndef FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE
#define FOONATHAN_MEMORY_RAW_ALLOCATOR_BASE

/// \file
/// \brief Utility base class generating functions.

#include <limits>

namespace foonathan { namespace memory
{
    /// \brief Base class that generates default implementations for the more repetitive functions.
    class raw_allocator_base
    {
    public:
        raw_allocator_base() = default;
        
        raw_allocator_base(const raw_allocator_base&) = delete;
        raw_allocator_base(raw_allocator_base&&) = default;
        
        raw_allocator_base& operator=(const raw_allocator_base &) = delete;
        raw_allocator_base& operator=(raw_allocator_base &&) = default;
        
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
