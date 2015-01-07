#ifndef FOONATHAN_MEMORY_POOL_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_HPP_INCLUDED

/// \file
/// \brief A memory pool.

#include <cassert>
#include <functional>
#include <new>

#include "detail/free_list.hpp"
#include "detail/memory_block.hpp"
#include "heap_allocator.hpp"
#include "tracking.hpp"

namespace foonathan { namespace memory
{
    /// \brief A memory pool.
    ///
    /// It manages memory blocks of fixed size.
    /// Allocating and deallocating such a block is really fast,
    /// but it can handle only one fixed size.
    /// Arrays are supported, too, but then deallocations are slower.<br>
    /// It is no \ref concept::RawAllocator, use adapters for it.
    /// \ingroup memory
    template <class ImplRawAllocator = heap_allocator>
    class memory_pool
    {
    public:
        //=== typedefs ===//
        /// \brief The allocator used to get the memory for this allocator.
        using impl_allocator = ImplRawAllocator;

        /// \brief The minimum element size, \c sizeof(char*).
        static constexpr auto min_element_size = sizeof(char*);

    private:
        using block_iter = typename detail::memory_block_list<impl_allocator>::iterator;

    public:
        //=== constructor ===//
        /// \brief Construct it given the start memory size and the element size.
        ///
        /// \param element_size is the size of each element being allocated/deallocated,
        /// it must be bigger than \ref min_element_size!
        /// \param block_size is the size of the first memory block being allocated,
        /// it is divided into smaller \c element_size big chunks being given to the user,
        /// thus it must be dividable by it. <br>
        /// For consistency, it is \b not the number of elements fitting into each block!
        /// The actual memory being allocated is slightly higher due to implementation issues.
        memory_pool(std::size_t element_size, std::size_t block_size,
                    impl_allocator alloc = impl_allocator())
        : mem_list_(block_size, std::move(alloc)),
          free_list_(check_element_size(element_size, block_size),
                     mem_list_.begin().memory(), block_size),
          top_mem_list_(mem_list_.begin()), capacity_(block_size)
        {}

        //=== fast allocation/deallocation ===//
        /// \brief Returns one memory element \ref element_size() big.
        ///
        /// If there are no more memory elements available,
        /// \ref allocator_growth_tracker is called and a new memory block allocated.
        /// After this process, \ref block_size() / \ref element_size() elements are availble. <br>
        /// The new memory block is inserted into the free list of memory blocks,
        /// it is inserted using the fast version that does not preserve ordering.
        /// This can lead to fragmentation of the free blocks,
        /// making array allocations more likely to fail. <br>
        /// This function provides the strong exception safety.
        void* allocate()
        {
            if (free_list_.empty())
            {
                detail::on_allocator_growth("foonathan::memory::memory_pool",
                                          this, block_size());
                reserve(1);
            }
            capacity_ -= element_size();
            return free_list_.allocate();
        }

         /// \brief Deallocates a single memory block previously returned from \ref allocate() or \ref allocate_ordered().
         ///
         /// The new memory block is simply pushed to the front of the free list,
         /// this makes it fast, but more difficult for array allocations.
         void deallocate(void *ptr)
         {
            assert(from_pool(ptr) && "pointer not allocated by this memory pool");
            free_list_.deallocate(ptr);
            capacity_ += element_size();
         }

        /// \brief Returns \c n continuous memory blocks each \ref element_size() big.
        ///
        /// This function otherwise does the same as \ref allocate()
        /// when there are no available in the current memory block. <br>
        /// It is assumed that the array fits into the new allocated block,
        /// that is, that it is smaller than \ref block_size().
        /// This function provides the strong exception safety.
        void* allocate_array(std::size_t n)
        {
            auto mem = free_list_.allocate(n);
            if (!mem)
            {
                detail::on_allocator_growth("foonathan::memory::memory_pool",
                                          this, block_size());
                reserve(1);
                mem = free_list_.allocate(n);
                if (!mem)
                    throw std::bad_array_new_length();
            }
            capacity_ -= n * element_size();
            return mem;
        }

        /// \brief Deallocates an array of memory blocks previously returned from \ref allocate_arry() or \ref allocate_array_ordered().
        ///
        /// The memory blocks are simply appended to the front of the free list.
        /// This makes it fast, but more difficult for further array allocations.
        void deallocate_array(void *ptr, std::size_t n) noexcept
        {
            assert(from_pool(ptr) && "pointer not allocated by this memory pool");
            free_list_.deallocate(ptr, n);
            capacity_ += n * element_size();
        }

        //=== ordered allocation/deallocation ===//
        /// \brief Returns one memory element \ref element_size() big.
        ///
        /// It does the same as \ref allocate() but preserves ordering when
        /// inserting an eventually necessarily new block.
        /// This leads to a slower growth, but makes array allocations more easier.<br>
        /// This function provides the strong exception safety.
        void* allocate_ordered()
        {
            if (free_list_.empty())
            {
                detail::on_allocator_growth("foonathan::memory::memory_pool",
                                          this, block_size());
                reserve_ordered(1);
            }
            capacity_ -= element_size();
            return free_list_.allocate();
        }

        /// \brief Deallocates a single memory block previously returned from \ref allocate() or \ref allocate_ordered().
         ///
         /// The new memory block is inserted in an appropriate position.
         /// This makes array allocations easier, but the function slower.
         void deallocate_ordered(void *ptr) noexcept
         {
            assert(from_pool(ptr) && "pointer not allocated by this memory pool");
            free_list_.deallocate_ordered(ptr);
            capacity_ += element_size();
         }

         /// \brief Returns \c n continuous memory blocks each \ref element_size() big.
        ///
        /// It does the same as \ref allocate() but preserves ordering when
        /// inserting an eventually necessarily new block.
        /// This leads to a slower growth, but makes array allocations more easier.<br>
        /// This function provides the strong exception safety.
        void* allocate_array_ordered(std::size_t n)
        {
            auto mem = free_list_.allocate(n);
            if (!mem)
            {
                detail::on_allocator_growth("foonathan::memory::memory_pool",
                                          this, block_size());
                reserve_ordered(1);
                mem = free_list_.allocate(n);
                if (!mem)
                    throw std::bad_array_new_length();
            }
            capacity_ -= n * element_size();
            return mem;
        }

        /// \brief Deallocates an array of memory blocks previously returned from \ref allocate_arry() or \ref allocate_array_ordered().
        ///
        /// The new memory block is inserted in an appropriate position.
        /// This makes array allocations easier, but the function slower.
        void deallocate_array_ordered(void *ptr, std::size_t n) noexcept
        {
            assert(from_pool(ptr) && "pointer not allocated by this memory pool");
            free_list_.deallocate_ordered(ptr, n);
        }

        //=== capacity ===//
        /// \brief Allocates \c n additional memory blocks.
        ///
        /// The new blocks are simply pushed to the front of the free list,
        /// this makes it fast, but more difficult for array allocations.<br>
        /// Since this is meant as a controlled grow,
        /// it does not notify \ref allocator_growth_tracker.<br>
        /// This function provides the strong exception safety.
        void reserve(std::size_t n)
        {
            reserve_mem(n); // exception safety, only function that can fail
            for (std::size_t i = 0; i != n; ++i)
            {
                free_list_.insert((++top_mem_list_).memory(),
                                  block_size());
            }
        }

        /// \brief Allocates \c n additional memory blocks.
        ///
        /// The new blocks are inserted in an appropriate position.
        /// This makes array allocations easier, but the function slower.<br>
        /// Otherwise, it behaves exactly as \ref reserve().
        void reserve_ordered(std::size_t n)
        {
            reserve_mem(n); // see reserve
            for (std::size_t i = 0; i != n; ++i)
            {
                ++top_mem_list_;
                auto mem = top_mem_list_.memory();
                free_list_.insert_ordered(mem,
                                          block_size());
            }
        }

        /// \brief Returns the number of memory blocks allocated.
        std::size_t no_blocks() const noexcept
        {
            return mem_list_.size();
        }

        /// \brief Returns the size of each memory block allocated when no more elements are availble.
        /// \note This is \b not the size of each memory block returned by the allocate functions!
        std::size_t block_size() const noexcept
        {
            return mem_list_.block_size();
        }

        /// \brief Returns the size of each memory block returned by the allocate functions.
        std::size_t element_size() const noexcept
        {
            return free_list_.element_size();
        }

        /// \brief Returns the amount of memory in bytes left in the pool without growing.
        ///
        /// Dividing this by \ref element_size() yields the number of elements available.
        std::size_t capacity() const noexcept
        {
            return capacity_;
        }

        //=== misc ===//
        /// \brief Returns the implementation allocator.
        impl_allocator& get_impl_allocator() noexcept
        {
            return mem_list_.get_allocator();
        }

    private:
        static std::size_t check_element_size(std::size_t element_size,
                                              std::size_t block_size)
        {
            assert(element_size >= min_element_size && "element size is too small");
            assert(block_size % element_size == 0 && "block size not dividable by element size");
            return element_size;
        }

        void reserve_mem(std::size_t n)
        {
            auto cur = top_mem_list_;
            for (std::size_t i = 0; i != n; ++i)
            {
                mem_list_.emplace_after(cur);
                ++cur;
            }
            capacity_ += n * block_size();
        }

        // does not catch every condition
        bool from_pool(void *ptr) const noexcept
        {
            auto begin = mem_list_.begin().memory();
            auto end = top_mem_list_.memory_end(block_size());
            return std::greater_equal<void*>()(ptr, begin)
                && std::less<void*>()(ptr, end);
        }

        detail::memory_block_list<impl_allocator> mem_list_;
        detail::free_memory_list free_list_;
        block_iter top_mem_list_;
        std::size_t capacity_;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_HPP_INCLUDED
