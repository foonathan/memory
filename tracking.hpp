#ifndef FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
#define FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED

/// \file
/// \brief Methods for tracking allocations.

#include <cstddef>

namespace foonathan { namespace memory
{
    /// \brief The heap allocation tracker.
    ///
    /// This function gets called when an heap (de-)allocation is made. <br>
    /// The default behavior is doing nothing. <br>
    /// \param allocation is \c true if it is allocated in \c new, \c false otherwise.
    /// \param ptr is the memory pointer being allocated/deallocated.
    /// \param size is the size of the memory block or \c 0 if not available.
    /// \note This function must not throw exceptions!
    /// \ingroup memory
    using heap_allocation_tracker = void(*)(bool allocation, void *ptr, std::size_t size);

    /// \brief Exchanges the \ref heap_allocation_tracker.
    ///
    /// This function is thread safe.
    /// \ingroup memory
    heap_allocation_tracker set_heap_allocation_tracker(heap_allocation_tracker t) noexcept;

    /// \brief Called when an allocator needs to expand itself.
    ///
    /// Since expansion is expensive due to a heap allocation,
    /// it might be prevented in a final game version. <br>
    /// You can either log the growth for further notice,
    /// or throw an exception to prevent it. <br>
    /// If this function does not return, the allocator won't be expanded and is therfore invalid (same as after \c std::move). <br>
    /// The default function does nothing.
    /// \param name is a string describing the type of allocator (e.g. "foonathan::memory::stack_allocator).
    /// Its lifetime is the same as the allocator.
    /// \param ptr is a pointer to the allocator.
    /// \param size is the block size of the allocator (which seems not to be sufficient).
    /// \ingroup memory
    using allocator_growth_tracker = void(*)(const char *name, void *ptr, std::size_t size);

    /// \brief Exchanges the \ref allocator_growth_tracker.
    ///
    /// This function is thread safe.
    /// \ingroup memory
    allocator_growth_tracker set_allocator_growth_tracker(allocator_growth_tracker t) noexcept;

    /// \cond impl
    namespace detail
    {
        void on_heap_alloc(bool allocation, void *ptr, std::size_t size) noexcept;
        void on_allocator_growth(const char *name, void *ptr, std::size_t size);
    } // namespace detail
    /// \endcond
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
