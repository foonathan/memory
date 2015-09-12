// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
#define FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED

/// \file
/// Class \ref foonathan::memory::tracked_allocator and related classes and functions.

#include <cstddef>

#include "detail/utility.hpp"
#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
    /// A \concept{concept_rawallocator,RawAllocator} adapter that tracks an implementation allocator.
    /// It wraps a \c RawAllocator that will be used as implementation allocator inside an arena.
    /// Any allocation and deallocation will forward to the growth and shrinking functions of the \concept{concept_tracker,deep tracker}.
    /// The class can then be passed as implementation allocator to an arena.
    /// \note It is recommended to use this class only with \ref tracked_allocator::make_deeply_tracked_allocator.
    /// \ingroup memory
    template <class Tracker, class ImplRawAllocator>
    class tracked_impl_allocator
    : FOONATHAN_EBO(allocator_traits<ImplRawAllocator>::allocator_type)
    {
        using traits = allocator_traits<ImplRawAllocator>;
    public:
        using allocator_type = typename traits::allocator_type ;
        using tracker = Tracker;

        using is_stateful = std::true_type;

        /// \effects Takes a \concept{concept_tracker,deep tracker} and the \concept{concept_rawallocator,RawAllocator}
        /// and wraps it.
        /// It will only store a pointer to the \c Trakcer to allow it being shared with the higher-level arena
        /// it is embedded in.
        tracked_impl_allocator(tracker &t, allocator_type allocator = {})
        : t_(&t),
          allocator_type(detail::move(allocator)) {}

        /// @{
        /// \effects Forwards to the allocation function of the implementation allocator
        /// and calls the <tt>Tracker::on_allocator_growth()</tt> function.
        /// \returns The result of the implementation allocator function.
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            auto mem = traits::allocate_node(*this, size, alignment);
            t_->on_allocator_growth(mem, size);
            return mem;
        }

        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            auto mem = traits::allocate_array(*this, count, size, alignment);
            t_->on_allocator_growth(mem, size * count);
            return mem;
        }
        /// @}

        /// @{
        /// \effects Forwards to the deallocation function of the implementation allocator
        /// after caling the <tt>Tracker::on_allocator_shrinking()</tt> function.
        void deallocate_node(void *ptr,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            t_->on_allocator_shrinking(ptr, size);
            traits::deallocate_node(*this, ptr, size, alignment);
        }

        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            t_->on_allocator_shrinking(ptr, size * count);
            traits::deallocate_array(*this, ptr, count, size, alignment);
        }
        /// @}

        /// @{
        /// \returns The result of the corresponding function the implementation allocator.
        std::size_t max_node_size() const
        {
            return traits::max_node_size(*this);
        }

        std::size_t max_array_size() const
        {
            return traits::max_array_size(*this);
        }

        std::size_t max_alignment() const
        {
            return traits::max_alignment(*this);
        }
        /// @}

    private:
        Tracker *t_;
    };

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
        explicit tracked_allocator(tracker t = {})
        : tracker(detail::move(t)) {}

        tracked_allocator(tracker t, allocator_type&& allocator)
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

#ifdef DOXYGEN
    private:
#endif
        template <class ImplRawAllocator, typename ... Args>
        tracked_allocator(tracker t, ImplRawAllocator impl,
                        Args&&... args)
        : tracker(detail::move(t)),
          allocator_type(detail::forward<Args>(args)...,
                tracked_impl_allocator<tracker, ImplRawAllocator>(*this, detail::move(impl)))
        {}
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

    /// \effects Takes a \concept{concept_tracker,deep tracker}, a \concept{concept_rawallocator,RawAllocator} and constructor arguments
    /// and creates a new \ref tracked_allocator that will track a new allocator object created with the constructor arguments
    /// and an implementation allocator that is the passed allocator wrapped in \ref tracked_impl_allocator with the same tracker.
    /// It assumes that the implementation allocator is the last constructor argument.
    /// \returns A \ref tracked_allocator that deeply tracks the given allocator type.
    /// \relates tracked_allocator
    template <template <class> class RawAllocator, class Tracker, class ImplRawAllocator, class ... Args>
    auto make_deeply_tracked_allocator(Tracker t, ImplRawAllocator impl, Args&&... args)
    -> tracked_allocator<Tracker, RawAllocator<tracked_impl_allocator<Tracker, ImplRawAllocator>>>
    {
        return {detail::move(t), detail::move(impl), detail::forward<Args&&>(args)...};
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
