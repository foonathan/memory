// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
#define FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::tracked_allocator and related classes and functions.

#include <cstddef>

#include "detail/utility.hpp"
#include "allocator_traits.hpp"
#include "memory_arena.hpp"

namespace foonathan { namespace memory
{
    /// A \concept{concept_rawallocator,RawAllocator} adapter that tracks another allocator using a \concept{concept_tracker,tracker}.
    /// It wraps another \c RawAllocator and calls the tracker function before forwarding to it.
    /// The class can then be used anywhere a \c RawAllocator is required and the memory usage will be tracked.
    /// \ingroup memory
    template <class Tracker, class RawAllocator>
    class tracked_allocator
    : FOONATHAN_EBO(Tracker, allocator_traits<RawAllocator>::allocator_type)
    {
        using traits = allocator_traits<RawAllocator>;
    public:
        using allocator_type = typename allocator_traits<RawAllocator>::allocator_type;
        using tracker = Tracker;

        using is_stateful = std::integral_constant<bool,
                            traits::is_stateful::value || !std::is_empty<Tracker>::value>;

        /// @{
        /// \effects Creates it by giving it a \concept{concept_tracker,tracker} and the tracked \concept{concept_rawallocator,RawAllocator}.
        /// It will embed both objects.
        explicit tracked_allocator(tracker t = {}) FOONATHAN_NOEXCEPT
        : tracker(detail::move(t)) {}

        tracked_allocator(tracker t, allocator_type&& allocator) FOONATHAN_NOEXCEPT
        : tracker(detail::move(t)), allocator_type(detail::move(allocator)) {}
        /// @}

        /// @{
        /// \effects Moving moves both the tracker and the allocator.
        tracked_allocator(tracked_allocator &&other) FOONATHAN_NOEXCEPT
        : tracker(detail::move(other)), allocator_type(detail::move(other)) {}

        tracked_allocator& operator=(tracked_allocator &&other) FOONATHAN_NOEXCEPT
        {
            tracker::operator=(detail::move(other));
            allocator_type::operator=(detail::move(other));
            return *this;
        }
        /// @}

        /// \effects Calls <tt>Tracker::on_node_allocation()</tt> and forwards to the allocator.
        /// \returns The result of <tt>allocate_node()</tt>
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            auto mem = traits::allocate_node(get_allocator(), size, alignment);
            this->on_node_allocation(mem, size, alignment);
            return mem;
        }

        /// \effects Calls <tt>Tracker::on_array_allocation()</tt> and forwards to the allocator.
        /// \returns The result of <tt>allocate_array()</tt>
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            auto mem = traits::allocate_array(get_allocator(), count, size, alignment);
            this->on_array_allocation(mem, count, size, alignment);
            return mem;
        }

        /// \effects Calls <tt>Tracker::on_node_deallocation()</tt> and forwards to the allocator's <tt>deallocate_node()</tt>.
        void deallocate_node(void *ptr,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            this->on_node_deallocation(ptr, size, alignment);
            traits::deallocate_node(get_allocator(), ptr, size, alignment);
        }

        /// \effects Calls <tt>Tracker::on_array_deallocation()</tt> and forwards to the allocator's <tt>deallocate_array()</tt>.
        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            this->on_array_deallocation(ptr, count, size, alignment);
            traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
        }

        /// @{
        /// \returns The result of the corresponding function on the wrapped allocator.
        std::size_t max_node_size() const
        {
            return traits::max_node_size(get_allocator());
        }

        std::size_t max_array_size() const
        {
            return traits::max_array_size(get_allocator());
        }

        std::size_t max_alignment() const
        {
            return traits::max_alignment(get_allocator());
        }
        /// @}

        /// @{
        /// \returns A (\c const) reference to the wrapped allocator.
        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }
        /// @}

        /// @{
        /// \returns A (\c const) reference to the tracker.
        tracker& get_tracker() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const tracker& get_tracker() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }
        /// @}
    };

    /// \effects Takes a \concept{concept_rawallocator,RawAllocator} and wraps it with a \concept{concept_tracker,tracker}.
    /// \returns A \ref tracked_allocator with the corresponding parameters forwarded to the constructor.
    /// \relates tracked_allocator
    template <class Tracker, class RawAllocator>
    auto make_tracked_allocator(Tracker t, RawAllocator &&alloc)
    -> tracked_allocator<Tracker, typename std::decay<RawAllocator>::type>
    {
        return tracked_allocator<Tracker, typename std::decay<RawAllocator>::type>{detail::move(t), detail::move(alloc)};
    }

    template <class Tracker, class BlockOrRawAllocator>
    class tracked_block_allocator
    : FOONATHAN_EBO(Tracker, make_block_allocator_t<BlockOrRawAllocator>)
    {
    public:
        using allocator_type = make_block_allocator_t<BlockOrRawAllocator>;
        using tracker = Tracker;

        explicit tracked_block_allocator(tracker t = {}) FOONATHAN_NOEXCEPT
        : tracker(detail::move(t)) {}

        tracked_block_allocator(tracker t, allocator_type &&alloc) FOONATHAN_NOEXCEPT
        : tracker(detail::move(t)), allocator_type(detail::move(alloc)) {}

        template <typename ... Args>
        tracked_block_allocator(std::size_t block_size, tracker t, Args&&... args)
        : tracker(detail::move(t)), allocator_type(block_size, detail::forward<Args>(args)...) {}

        memory_block allocate_block()
        {
            auto block = allocator_type::allocate_block();
            this->on_allocator_growth(block.memory, block.size);
            return block;
        }

        void deallocate_block(memory_block block) FOONATHAN_NOEXCEPT
        {
            this->on_allocator_shrinking(block.memory, block.size);
            allocator_type::deallocate_block(block);
        }

        std::size_t next_block_size() const FOONATHAN_NOEXCEPT
        {
            return allocator_type::next_block_size();
        }

        allocator_type& get_allocator() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const allocator_type& get_allocator() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        tracker& get_tracker() FOONATHAN_NOEXCEPT
        {
            return *this;
        }

        const tracker& get_tracker() const FOONATHAN_NOEXCEPT
        {
            return *this;
        }
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
