#ifndef FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
#define FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED

/// \file
/// \brief Methods for tracking allocations.

#include <cstddef>

#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
    /// \brief The heap allocation tracker.
    /// \detail This function gets called when an heap (de-)allocation is made.
    /// It can be used to log the (slower) heap allocations.
    /// A heap allocation is an allocation via \ref heap_allocator or \ref new_allocator.<br>
    /// The default behavior is doing nothing. <br>
    /// \param allocation is \c true if it is an allocation, false if deallocation.
    /// \param ptr is the memory pointer being allocated/deallocated.
    /// \param size is the size of the memory block or \c 0 if not available.
    /// \note This function must not throw exceptions!
    /// \ingroup memory
    using heap_allocation_tracker = void(*)(bool allocation, void *ptr, std::size_t size);

    /// \brief Exchanges the \ref heap_allocation_tracker.
    /// \detail This function is thread safe.
    /// \ingroup memory
    heap_allocation_tracker set_heap_allocation_tracker(heap_allocation_tracker t) noexcept;

    /// \brief Called when an allocator needs to expand itself after initial allocations.
    /// \detail Some allocators (e.g. \ref memory_stack, \ref memory_pool) consists of a list of big memory blocks.
    /// When they are exhausted, new ones are allocated. <br>
    /// Use this function to monitor growth. It must not throw an exception however.<br>
    /// The default function does nothing.
    /// \param name is a string describing the type of allocator (e.g. "foonathan::memory::stack_allocator).
    /// Its lifetime is the same as the allocator.
    /// \param ptr is the address of the allocator or submember.
    /// It can only be used to distinguish multiple allocators when logging.
    /// \param size is the old block size of the allocator (which seems not to be sufficient).
    /// \ingroup memory
    using allocator_growth_tracker = void(*)(const char *name, void *ptr, std::size_t size);

    /// \brief Exchanges the \ref allocator_growth_tracker.
    /// \detail This function is thread safe.
    /// \ingroup memory
    allocator_growth_tracker set_allocator_growth_tracker(allocator_growth_tracker t) noexcept;

    /// \brief A wrapper around an \ref concept::RawAllocator that allows logging.
    /// \detail The \c Tracker must provide the following, \c noexcept functions:
    /// * \c on_node_allocation(void *memory, std::size_t size, std::size_t alignment)
    /// * \c on_node_deallocation(void *memory, std::size_t size, std::size_t alignment)
    /// * \c on_array_allocation(void *memory, std::size_t count, std::size_t size, std::size_t alignment)
    /// * \c on_array_deallocation(void *memory, std::size_t count, std::size_t size, std::size_t alignment) 
    /// The \c RawAllocator functions are called via the \ref allocator_traits.
    /// \ingroup memory
    template <class RawAllocator, class Tracker>
    class tracked_allocator : RawAllocator, Tracker
    {
        using traits = allocator_traits<RawAllocator>;
    public:
        using raw_allocator = RawAllocator;
        using tracker = Tracker;  

        /// \brief The allocator is stateful if the \ref raw_allocator is or the \ref tracker non-empty.
        using is_stateful = std::integral_constant<bool,
                            traits::is_stateful::value || !std::is_empty<Tracker>::value>;
        
        tracked_allocator(raw_allocator allocator = {},
                            tracker t = {})
        : raw_allocator(std::move(allocator)),
          tracker(std::move(t)) {}
        
        /// @{
        /// \brief (De-)Allocation functions call the appropriate tracker function, too.
        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            auto mem = traits::allocate_node(get_allocator(), size, alignment);
            this->on_node_allocation(mem, size, alignment);
            return mem;
        }
        
        void* allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
        {
            auto mem = traits::allocate_array(get_allocator(), count, size, alignment);
            this->on_array_allocation(mem, count, size, alignment);
            return mem;
        }
        
        void deallocate_node(void *ptr,
                              std::size_t size, std::size_t alignment) noexcept
        {
            this->on_node_deallocation(ptr, size, alignment);
            traits::deallocate_node(get_allocator(), ptr, size, alignment);
        }
        
        void deallocate_array(void *ptr, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            this->on_array_deallocation(ptr, count, size, alignment);
            traits::deallocate_array(get_allocator(), ptr, count, size, alignment);
        }
        /// @}        
        
        /// @{
        /// \brief Forwards to the allocator.
        std::size_t max_node_size() const noexcept
        {
            return traits::max_node_size(get_allocator());
        }
        
        std::size_t max_array_size() const noexcept
        {
            return traits::max_array_size(get_allocator());
        }
        
        std::size_t max_alignment() const noexcept
        {
            return traits::max_alignment(get_allocator());
        }
        /// @}
        
        /// @{
        /// \brief Returns a reference to the allocator.
        raw_allocator& get_allocator() noexcept
        {
            return *this;
        }
        
        const raw_allocator& get_allocator() const noexcept
        {
            return *this;
        }
        /// @}
        
        /// @{
        /// \brief Returns a reference to the tracker.
        tracker& get_tracker() noexcept
        {
            return *this;
        }
        
        const tracker& get_tracker() const noexcept
        {
            return *this;
        }
        /// @}
    };

    /// \cond impl
    namespace detail
    {
        void on_heap_alloc(bool allocation, void *ptr, std::size_t size) noexcept;
        void on_allocator_growth(const char *name, void *ptr, std::size_t size) noexcept;
    } // namespace detail
    /// \endcond
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_TRACKING_HPP_INCLUDED
