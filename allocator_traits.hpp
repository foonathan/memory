#ifndef FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED

/// \file
/// \brief Allocator traits class template.

#include <cstddef>
#include <memory>
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

        static std::size_t max_node_size(const allocator_state &state) noexcept
        {
            return state.max_node_size();
        }

        static std::size_t max_array_size(const allocator_state &state) noexcept
        {
            return state.max_array_size();
        }
        
        static std::size_t max_alignment(const allocator_state &state) noexcept
        {
            return state.max_alignment();
        }
    };
    
    /// \brief Provides all traits functions for \c std::allocator types.
    /// \detail Inherit from it when specializing the \ref allocator_traits for such allocators.<br>
    /// It uses the std::allocator_traits to call the functions.
    /// \ingroup memory
    template <class StdAllocator>
    class allocator_traits_std_allocator
    {
    public:
        /// \brief The state type is the Allocator rebind for \c char.
        using allocator_state = typename std::allocator_traits<StdAllocator>::
                                    template rebind_alloc<char>;
        
        /// \brief Assume it is not stateful when std::is_empty.
        using is_stateful = std::integral_constant<bool, !std::is_empty<allocator_state>::value>;
        
    private:
        using std_traits = std::allocator_traits<allocator_state>;
        using state_argument = typename std::conditional<is_stateful::value,
                                    allocator_state&, allocator_state>::type;
        
    public:        
        /// @{
        /// \brief Allocation functions forward to \c allocate().
        /// \detail They request a char-array of sufficient length.<br>
        /// Alignment is ignored.
        static void* allocate_node(state_argument state,
                                std::size_t size, std::size_t)
        {
            return std_traits::allocate(state, size);
        }

        static void* allocate_array(state_argument state, std::size_t count,
                             std::size_t size, std::size_t)
        {
            return std_traits::allocate(state, count * size);
        }
        /// @}

        /// @{
        /// \brief Deallocation functions forward to \c deallocate().
        static void deallocate_node(state_argument state,
                    void *node, std::size_t size, std::size_t) noexcept
        {
            std_traits::deallocate(state, static_cast<typename std_traits::pointer>(node), size);
        }

        static void deallocate_array(state_argument state, void *array, std::size_t count,
                              std::size_t size, std::size_t) noexcept
        {
            std_traits::deallocate(state, static_cast<typename std_traits::pointer>(array), count * size);
        }
        /// @}

        /// @{
        /// \brief The maximum size forwards to \c max_size().
        static std::size_t max_node_size(const allocator_state &state) noexcept
        {
            return std_traits::max_size(state);
        }

        static std::size_t max_array_size(const allocator_state &state) noexcept
        {
            return std_traits::max_size(state);
        }
        /// @}
        
        /// \brief Maximum alignment is \c alignof(std::max_align_t).
        static std::size_t max_alignment(const allocator_state &) noexcept
        {
            return alignof(std::max_align_t);
        }
    };
    
    /// \brief Specialization of \ref allocator_traits for \c std::allocator.
    /// \ingroup memory
    // implementation note: std::allocator can have any number of implementation defined, defaulted arguments
    template <typename ... ImplArguments>
    class allocator_traits<std::allocator<ImplArguments...>>
    : public allocator_traits_std_allocator<std::allocator<ImplArguments...>>
    {};
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_TRAITS_HPP_INCLUDED