// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TEMPORARY_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_TEMPORARY_ALLOCATOR_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::temporary_allocator and related functions.

#include FOONATHAN_THREAD_LOCAL_HEADER

#include "allocator_traits.hpp"
#include "config.hpp"
#include "memory_stack.hpp"

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

    /// A stateful \concept{concept_rawallocator,RawAllocator} that handles temporary allocations.
    /// It works similar to \c alloca() but uses a thread local \ref memory_stack for the allocations,
    /// instead of the actual program stack.
    /// This avoids the stack overflow error and is portable,
    /// with a similar speed.
    /// All allocations done in the scope of the allocator object are automatically freed when the object is destroyed.
    /// \ingroup memory
    class temporary_allocator
    {
    public:
        /// The type of the handler called when the internal \ref memory_stack grows.
        /// It gets the size of the new block that will be allocated.
        /// \requiredbe The handler shall log the growth, throw an exception or aborts the program.
        /// If this function does not return, the growth is prevented but the allocator unusable until memory is freed.
        /// \defaultbe The default handler does nothing.
        using growth_tracker = void(*)(std::size_t size);

        /// \effects Sets \c h as the new \ref growth_tracker.
        /// A \c nullptr sets the default \ref growth_tracker.
        /// Each thread has its own, separate tracker.
        /// \returns The previous \ref growth_tracker. This is never \c nullptr.
        static growth_tracker set_growth_tracker(growth_tracker t) FOONATHAN_NOEXCEPT;

        /// \returns The current \ref growth_tracker. This is never \c nullptr.
        static growth_tracker get_growth_tracker() FOONATHAN_NOEXCEPT;

        /// \effects Unwinds the \ref memory_stack to the point where it was upon creation.
        ~temporary_allocator() FOONATHAN_NOEXCEPT;

        /// @{
        /// \effects Makes the destination allocator object the active allocator.
        /// Allocation must only be done from the active allocator object.
        temporary_allocator(temporary_allocator &&other) FOONATHAN_NOEXCEPT;
        temporary_allocator& operator=(temporary_allocator &&other) FOONATHAN_NOEXCEPT;
        /// @}

        /// \effects Allocates memory from the internal \ref memory_stack by forwarding to it.
        /// \returns The result of \ref memory_stack::allocate().
        /// \requires This function must be called on the active allocator object.
        void* allocate(std::size_t size, std::size_t alignment);

    private:
        temporary_allocator(std::size_t size) FOONATHAN_NOEXCEPT;

        static FOONATHAN_THREAD_LOCAL const temporary_allocator *top_;
        memory_stack<>::marker marker_;
        const temporary_allocator *prev_;
        bool unwind_;

        friend temporary_allocator make_temporary_allocator(std::size_t size) FOONATHAN_NOEXCEPT;
    };

    /// \effects Creates a new \ref temporary_allocator object and makes it the active object.
    /// If this is the first call of this function in a thread it will create the internal \ref memory_stack with the given size.
    /// If there is never a call to this function in a thread, it will never be created.
    /// If this is not the first call, the size parameter will be ignored.
    /// The internal stack will be destroyed when the current thread ends.
    /// Without growth there will be at most one heap allocation per thread.
    /// \returns A new \ref temporary_allocator object that is the active object.
    /// \requires The result must be stored on the program stack.
    /// \relates temporary_allocator
    inline temporary_allocator make_temporary_allocator(std::size_t size = 4096u) FOONATHAN_NOEXCEPT
    {
        return {size};
    }

    /// Specialization of the \ref allocator_traits for \ref temporary_allocator classes.
    /// \note It is not allowed to mix calls through the specialization and through the member functions,
    /// i.e. \ref temporary_allocator::allocate() and this \c allocate_node().
    /// \ingroup memory
    template <>
    class allocator_traits<temporary_allocator>
    {
    public:
        using allocator_type = temporary_allocator;
        using is_stateful = std::true_type;

        /// \returns The result of \ref temporary_allocator::allocate().
        static void* allocate_node(allocator_type &state, std::size_t size, std::size_t alignment)
        {
            detail::check_allocation_size(size, max_node_size(state),
                {FOONATHAN_MEMORY_LOG_PREFIX "::temporary_allocator", &state});
            return state.allocate(size, alignment);
        }

        /// \returns The result of \ref temporary_allocator::allocate().
        static void* allocate_array(allocator_type &state, std::size_t count,
                                std::size_t size, std::size_t alignment)
        {
            return allocate_node(state, count * size, alignment);
        }

        /// @{
        /// \effects Does nothing besides bookmarking for leak checking, if that is enabled.
        /// Actual deallocation will be done automatically if the allocator object goes out of scope.
        static void deallocate_node(const allocator_type &,
                    void *, std::size_t, std::size_t) FOONATHAN_NOEXCEPT {}

        static void deallocate_array(const allocator_type &,
                    void *, std::size_t, std::size_t, std::size_t) FOONATHAN_NOEXCEPT {}
        /// @}

        /// @{
        /// \returns The maximum size which is \ref memory_stack::next_capacity() of the internal stack.
        static std::size_t max_node_size(const allocator_type &state) FOONATHAN_NOEXCEPT;

        static std::size_t max_array_size(const allocator_type &state) FOONATHAN_NOEXCEPT
        {
            return max_node_size(state);
        }
        /// @}

        /// \returns The maximum possible value since there is no alignment restriction
        /// (except indirectly through \ref memory_stack::next_capacity()).
        static std::size_t max_alignment(const allocator_type &) FOONATHAN_NOEXCEPT
        {
            return std::size_t(-1);
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_TEMPORARY_ALLOCATOR_HPP_INCLUDED
