#ifndef FOONATHAN_MEMORY_POOL_COLLECTION_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_COLLECTION_HPP_INCLUDED

/// \file
/// \brief A class managing pools of different sizes.

#include <type_traits>
#include <vector>

#include "allocator_adapter.hpp"
#include "heap_allocator.hpp"
#include "pool.hpp"

namespace foonathan { namespace memory
{
    /// \brief A \ref concept::RawAllocator managing various fixed sized \ref memory_pool.
    ///
    /// It stores a collection of pools, the element size of each one being a power of two.
    /// \ingroup memory
    class memory_pool_collection : heap_allocator
    {
    public:
        using stateful = std::true_type;

        //=== constructor ===//
        /// \brief Creates a new pool collection.
        ///
        /// Initially, the collection is empty.
        /// \param no_elements_pool is the number of element a newly created \ref memory_pool should have,
        /// this is only used when automatic creation of pools in \ref allocate is necessary.
        /// \param no_pools is the maximum number of pools the collection will ever contain,
        /// this is just a hint, growing is possible, note that no pool will be created.
        memory_pool_collection(std::size_t no_elements_pool,
                               std::size_t no_pools);

        //=== allocation/deallocation ===//
        /// \brief Allocates a continuous memory blocks of \c n bytes.
        ///
        /// Internally, calls \ref memory_pool::allocate() of a pool with an element size
        /// the next power of two after \c n (or \c n if it is a power of two).<br>
        /// If there is no such pool, it will be created on-the-fly,
        /// this also triggers \ref allocator_growth_tracker
        /// giving it as size the power of two not there.<br>
        /// If the allocator does not need to grow, the complexity is constant.<br>
        void* allocate(std::size_t size, std::size_t);

        /// \brief Deallocates a previously allocated memory block.
        void deallocate(void *mem, std::size_t size, std::size_t) noexcept;

        //=== pool management ===//
        /// \brief Inserts a new \ref memory_pool.
        /// \param el_size must be a power of two.
        /// \param no_elements is the number of elements in the pool.
        /// \note The pool must not already be in the pool!
        void insert_pool(std::size_t el_size, std::size_t no_elements);

        /// \brief Checks whether there is a \ref memory_pool of given element size.
        bool has_pool(std::size_t el_size) const noexcept;

        /// \brief Access the pool with given element size.
        memory_pool<>& get_pool(std::size_t el_size) noexcept;

    private:
        void insert_pool(std::size_t index, std::size_t el_size, std::size_t no_el);
        bool has_pool(std::size_t index, std::size_t el_size) const noexcept;

        std::vector<memory_pool<>,
                    raw_allocator_allocator<memory_pool<>, heap_allocator>> pools_;
        std::size_t no_elements_pool_;
    };
}} // namespace foonathan::portal

#endif // FOONATHAN_MEMORY_POOL_COLLECTION_HPP_INCLUDED
