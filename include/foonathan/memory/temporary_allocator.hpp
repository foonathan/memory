// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TEMPORARY_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_TEMPORARY_ALLOCATOR_HPP_INCLUDED

#include "allocator_traits.hpp"
#include "config.hpp"
#include "stack_allocator.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        static class temporary_allocator_dtor_t
        {
        public:
            temporary_allocator_dtor_t() FOONATHAN_NOEXCEPT;
            ~temporary_allocator_dtor_t() FOONATHAN_NOEXCEPT;
        private:
            static FOONATHAN_THREAD_LOCAL std::size_t nifty_counter_;
        } temporary_allocator_dtor;
    } // namespace detail

    /// \brief A memory allocator for temporary allocations.
    /// \details It is similar to \c alloca() but portable.
    /// It uses a \c thread_local \ref memory_stack<> for the allocation.<br>
    /// It is no \ref concept::RawAllocator, but the \ref allocator_traits are specialized for it.<br>
    /// \ingroup memory
    class temporary_allocator
    {
    public:
        /// \brief The type of the growth tracker.
        /// \details It gets called when the internal \ref memory_stack<> needs to grow.
        /// It gets the size of the new block that will be allocated.
        /// If this function doesn't return, growth is prevented but the allocator unusable.<br>
        /// Each thread has its own internal stack and thus own tracker.
        using growth_tracker = void(*)(std::size_t size);

        /// \brief Exchanges the \ref growth_tracker.
        static growth_tracker set_growth_tracker(growth_tracker t) FOONATHAN_NOEXCEPT;

        temporary_allocator(temporary_allocator &&other) FOONATHAN_NOEXCEPT;
        ~temporary_allocator() FOONATHAN_NOEXCEPT;

        temporary_allocator& operator=(temporary_allocator &&other) FOONATHAN_NOEXCEPT;

        /// \brief Allocates temporary memory of given size and alignment.
        /// \details It will be deallocated when the allocator goes out of scope.<br>
        /// For that reason, allocation must be made from the most recent created allocator.
        void* allocate(std::size_t size, std::size_t alignment);

    private:
        temporary_allocator(std::size_t size) FOONATHAN_NOEXCEPT;

        static FOONATHAN_THREAD_LOCAL const temporary_allocator *top_;
        memory_stack<>::marker marker_;
        const temporary_allocator *prev_;
        bool unwind_;

        friend temporary_allocator make_temporary_allocator(std::size_t size) FOONATHAN_NOEXCEPT;
    };

    /// \brief Creates a new \ref temporary_allocator.
    /// \details This is the only way to create to avoid accidental creation not on the stack.<br>
    /// The internal stack allocator will only be created in a thread if there is at least one call to this function.
    /// If it is the call that actually creates it, the stack has the initial size passed to it.
    /// The stack will be destroyed when the current thread ends, so there is - no growth needed - only one heap allocation per thread.
    /// \relates temporary_allocator
    inline temporary_allocator make_temporary_allocator(std::size_t size = 4096u) FOONATHAN_NOEXCEPT
    {
        return {size};
    }

    /// \brief Specialization of the \ref allocator_traits for \ref temporary_allocator.
    /// \details This allows passing a pool directly as allocator to container types.
    /// \ingroup memory
    template <>
    class allocator_traits<temporary_allocator>
    {
    public:
        using allocator_type = temporary_allocator;
        using is_stateful = std::true_type;

        /// @{
        /// \brief Allocation function forward to the temporary allocator for array and node.
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
        /// \brief Deallocation functions do nothing, everything is freed on scope exit.
        static void deallocate_node(const allocator_type &,
                    void *, std::size_t, std::size_t) FOONATHAN_NOEXCEPT {}

        static void deallocate_array(const allocator_type &,
                    void *, std::size_t, std::size_t, std::size_t) FOONATHAN_NOEXCEPT {}
        /// @}

        /// @{
        /// \brief The maximum size is the equivalent of the capacity left in the next block of the internal \ref memory_stack<>.
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT;

        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return max_node_size(state);
        }
        /// @}

        /// \brief There is no maximum alignment (except indirectly through \ref max_node_size()).
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_TEMPORARY_ALLOCATOR_HPP_INCLUDED
