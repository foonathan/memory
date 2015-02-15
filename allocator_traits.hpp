#ifndef FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED

/// \file
/// \brief Allocator traits class template.

#include <type_traits>

namespace foonathan { namespace memory
{
    /// \brief Default traits for \ref concept::RawAllocator classes.
    /// \detail Specialize it for own classes.
    /// \ingroup memory
    template <class Allocator>
    class allocator_traits
    {
        using state_argument = typename std::conditional<Allocator::is_stateful::value,
                                    Allocator&, const Allocator&>::type;
    public:
        /// \brief The state type that need to be stored.
        /// \detail It will be passed to all the other functions.
        /// \ref raw_allocator_adapter will store a reference to it.
        /// For non stateful allocators it will be default-constructed on the fly.<br>
        /// Default is the allocator type itself.
        using allocator_state = Allocator;
        
        using is_stateful = typename Allocator::is_stateful;
        
        static void* allocate_node(state_argument state,
                                std::size_t size, std::size_t alignment)
        {
            return state.allocate_node(size, alignment);
        }

        static void* allocate_array(state_argument state, std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            return state.allocate_array(count, size, alignment);
        }

        static void deallocate_node(state_argument state,
                    void *node, std::size_t size, std::size_t alignment) noexcept
        {
            state.deallocate_node(node, size, alignment);
        }

        static void deallocate_array(state_argument state, void *array, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            state.deallocate_array(array, count, size, alignment);
        }

        static std::size_t max_node_size(const allocator_state &state)
        {
            return state.max_node_size();
        }

        static std::size_t max_array_size(const allocator_state &state)
        {
            return state.max_array_size();
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED