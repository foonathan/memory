#ifndef FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A stack allocator.

#include <cassert>
#include <cstdint>
#include <type_traits>

#include "detail/align.hpp"
#include "detail/block_list.hpp"
#include "allocator_traits.hpp"
#include "heap_allocator.hpp"
#include "raw_allocator_base.hpp"

namespace foonathan { namespace memory
{    
    /// \brief A memory stack.
    ///
    /// Allows fast memory allocations but deallocation is only possible via markers.
    /// All memory after a marker is then freed, too.<br>
    /// It is no \ref concept::RawAllocator, but the \ref allocator_traits are specialized for it.<br>
    /// It allocates big blocks from an implementation allocator.
    /// If their size is sufficient, allocations are fast.
    /// \ingroup memory
    template <class RawAllocator = heap_allocator>
    class memory_stack
    {
    public:
        /// \brief The implementation allocator.
        using impl_allocator = RawAllocator;
    
        /// \brief Constructs it with a given start block size.
        /// \detail The first memory block is allocated, the block size can change.
        explicit memory_stack(std::size_t block_size,
                        impl_allocator allocator = impl_allocator())
        : list_(block_size, std::move(allocator))
        {
            allocate_block();
        }
        
        /// \brief Allocates a memory block of given size and alignment.
        /// \detail If it does not fit into the current block, a new one will be allocated.
        /// The new block must be big enough for the requested memory.
        void* allocate(std::size_t size, std::size_t alignment)
        {
            auto offset = align_offset(alignment);
            if (offset + size > capacity())
            {
                allocate_block();
                offset = align_offset(alignment);
                assert(offset + size <= capacity() && "block size still too small");
            }
            // now we have sufficient size
            cur_ += offset; // align
            auto memory = cur_;
            cur_ += size; // bump
            return memory;
        }
        
        /// \brief Marker type for unwinding.
        class marker
        {
            std::size_t index;
            // store both and cur_end to replicate state easily
            char *cur, *cur_end;
            
            marker(std::size_t i, char *cur, char *cur_end) noexcept
            : index(i), cur(cur), cur_end(cur_end) {}
        };
        
        /// \brief Returns a marker to the current top of the stack.
        marker top() const noexcept
        {
            return {list_.size() - 1, cur_, cur_end_};
        }
        
        /// \brief Unwinds the stack to a certain marker.
        /// \detail It must be less than the previous one.
        /// Any access blocks are freed.
        void unwind(marker m) noexcept
        {
            auto diff = list_.size() - m.index - 1;
            for (auto i = 0u; i != diff; ++i)
                list_.deallocate();
            cur_     = m.cur_;
            cur_end_ = m.cur_end_;
        }
        
        /// \brief Returns the capacity remaining in the current block.
        std::size_t capacity() const noexcept
        {
            return cur_end_ - cur_;
        }
        
        /// \brief Returns the size of the memory block available after the capacity() is exhausted.
        std::size_t next_capacity() const noexcept
        {
            return list_.next_block_size();
        }
        
        /// \brief Returns the \ref impl_allocator.
        impl_allocator& get_impl_allocator() noexcept
        {
            return list_.get_allocator();
        }
        
    private:
        std::size_t align_offset(std::size_t alignment) const noexcept
        {
            return detail::align_offset(cur_, alignment);
        }
        
        void allocate_block()
        {
            auto block = list_.allocate("foonathan::memory::memory_stack");
            cur_ = static_cast<char*>(block.memory);
            cur_end_ = cur_ + block.size;
        }
    
        detail::block_list<impl_allocator> list_;
        char *cur_, *cur_end_;
    };
    
    /// \brief Specialization of the \ref allocator_traits for a \ref memory_state.
    /// \detail This allows passing a state directly as allocator to container types.
    /// \ingroup memory
    template <class ImplRawAllocator>
    class allocator_traits<memory_stack<ImplRawAllocator>>
    {
    public:
        using allocator_type = memory_stack<ImplRawAllocator>;
        using is_stateful = std::true_type;
        
        /// @{
        /// \brief Allocation function forward to the stack for array and node.
        static void* allocate_node(allocator_type &state, std::size_t size, std::size_t alignment)
        {
            assert(size <= max_node_size(state) && "invalid node size");
            return state.allocate(size, alignment);
        }
        
        static void* allocate_array(allocator_type &state, std::size_t count,
                                std::size_t size, std::size_t alignment)
        {
            return allocate_node(state, count * size, alignment);
        }
        /// @}
        
        /// @{
        /// \brief Deallocation functions do nothing, use unwinding on the stack to free memory.
        static void deallocate_node(const allocator_type &,
                    void *, std::size_t, std::size_t) noexcept {}
        
        static void deallocate_array(const allocator_type &,
                    void *, std::size_t, std::size_t, std::size_t) noexcept {}
        /// @}
        
        /// @{
        /// \brief The maximum size is the equivalent of the \ref next_capacity().
        static std::size_t max_node_size(const allocator_type &state) noexcept
        {
            return state.next_capacity();
        }
        
        static std::size_t max_array_size(const allocator_type &state) noexcept
        {
            return state.next_capacity();
        }
        /// @}
        
        /// \brief There is no maximum alignment (except indirectly through \ref next_capacity()).
        static std::size_t max_alignment(const allocator_type &) noexcept
        {
            return 0;
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
