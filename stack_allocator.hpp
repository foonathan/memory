#ifndef FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief Stack allocators.

#include <cassert>
#include <type_traits>

#include "detail/memory_block.hpp"
#include "heap_allocator.hpp"
#include "tracking.hpp"

namespace foonathan { namespace memory
{
    /// \brief A \ref concept::RawAllocator that allocates memory in a stack.
    ///
    /// It allocates by moving a pointer in a memory block.
    /// The deallocation function does nothing, you must explicitly move the pointer back. <br>
    /// This is very efficient for all kind of allocation sizes,
    /// the downside is the fixed deallocation order by unwinding.
    /// \ingroup memory
    template <class ImplRawAllocator = heap_allocator>
    class stack_allocator
    {
    public:
        //=== typedefs ===//
        /// \brief The allocator used to get the memory for this allocator.
        using impl_allocator = ImplRawAllocator;

        using stateful = std::true_type;

    private:
        using block_iter = typename detail::memory_block_list<impl_allocator>::iterator;

    public:
        //=== constructor ===//
        /// \brief Construct it giving a start block size.
        ///
        /// The actual memory being allocated is slightly higher,
        /// due to some implementation overhead.
        explicit stack_allocator(std::size_t block_size,
                                 impl_allocator alloc = impl_allocator())
        : list_(block_size, std::move(alloc)),
          cur_block_(list_.begin()), cur_mem_(cur_block_.memory())
        {}

        //=== allocation/deallocation ===//
        /// \brief Aligns the top of the stack to a certain alignment.
        ///
        /// The stack top is moved to a certain alignment position. <br>
        /// This is especially useful when you need \ref get_memory(),
        /// where you need to manually calculate the offset to fulfill alignment.
        /// \param alignment must be a power of two.
        /// \return
        void align_top(std::size_t alignment)
        {
           allocate(0, alignment);
        }

        /// \brief Allocates raw memory.
        ///
        /// If the requested memory does not fit into the current memory block,
        /// \ref allocator_growth_tracker is called and a new one is allocated.
        /// The new block must be large enough to hold the memory! <br>
        /// This function provides the strong exception safety.
        /// \param alignment must be a power of two.
        void* allocate(std::size_t size, std::size_t alignment)
        {
            auto available = capacity();
            auto offset = align(cur_mem_, alignment);
            if (size + offset > available)
            {
                detail::on_allocator_growth("foonathan::memory::stack_allocator", this,
                                          block_size());
                reserve(1);
                offset = align(cur_mem_, alignment);
                assert(size + offset <= capacity() && "can't fit element into new block neither");
            }
            cur_mem_ += offset;
            auto mem = cur_mem_;
            cur_mem_ += size;
            return mem;
        }

        /// \brief Releases raw memory.
        ///
        /// Since the order of deallocation is fixed,
        /// this function can't do anything (use \ref unwind).
        void deallocate(void *, std::size_t, std::size_t) noexcept {}

        //=== unwinding ===//
        /// \brief A marker storing a state of a stack.
        class marker
        {
            block_iter block;
            char *memory;

            marker(block_iter b, char *mem) noexcept
            : block(std::move(b)), memory(mem) {}

            friend stack_allocator;

        public:
            marker() noexcept
            : marker({}, nullptr) {}
        };

        /// \brief Returns a \ref marker to the current top of the stack.
        marker top() const noexcept
        {
            return {cur_block_, cur_mem_};
        }

        /// \brief Returns a \ref marker to the bottom of the stack.
        marker bottom() const noexcept
        {
            return {list_.begin(), list_.begin().memory()};
        }

        /// \brief Returns the memory block at a given positive offset from a marker.
        ///
        /// This is in theory the same as \c top + \c offset, but takes growing into account. <br>
        /// You can use this function to get to a memory block without knowing its exact address,
        /// but the blocks being allocated between the marker \c m and the target.
        /// \c offset is not only the sum of the sizes of these blocks, but the sum of the offsets to fulfill alignment, too.
        /// You can ensure a given start alignment via \ref align_top(),
        /// the rest is calculation.
        /// It is guaranteed that the allocator will leave only holes when alignment is otherwise not guaranteed.
        void* get_memory(marker m, std::size_t offset) const noexcept
        {
            for (std::size_t cap = m.block.memory_end(block_size()) - m.memory;
                 cap <= offset; ++m.block, m.memory = m.block.memory())
                 offset -= cap;
            return m.memory + offset;
        }

        /// \brief Similar to \ref get_memory(), but without offset.
        ///
        /// It just returns the address of the marker.
        void* get_memory(marker m) const noexcept
        {
            return m.memory;
        }

        /// \brief Unwinds the stack to certain marker position.
        ///
        /// The marker must be returned by a previous call to \ref top()
        /// and the stack must not has deallocated before that position
        /// (even if further allocations have moved the top again after that marker!)
        /// This function does not free any blocks, use \ref shrink_to_fit for that.
        /// \note No destructors are called.
        void unwind(const marker &m) noexcept
        {
            cur_block_ = m.block;
            cur_mem_ = m.memory;
        }

        /// \brief Clears the stack back to zero size.
        /// \note No destructors are called.
        void clear() noexcept
        {
            unwind(bottom());
        }

        //=== capacity ===//
        /// \brief Allocates \c n additional blocks.
        ///
        /// Since this is meant as a controlled grow,
        /// it does not notify \ref allocator_growth_tracker.<br>
        /// This function provides the strong exception safety.
        void reserve(std::size_t n)
        {
            auto cur = cur_block_; // exception safety!
            for (std::size_t i = 0; i != n; ++i)
            {
                list_.emplace_after(cur);
                ++cur;
            }
            cur_mem_ = cur.memory();
            cur_block_ = cur;
        }

        /// \brief Releases unused memory blocks if any.
        ///
        /// When upon unwinding memory blocks get unused, they are not freed,
        /// this function does it explicitly. <br>
        void shrink_to_fit() noexcept
        {
            list_.erase_all_after(cur_block_);
        }

        /// \brief Returns the number of blocks currently allocated.
        std::size_t no_blocks() const noexcept
        {
            return list_.size();
        }

        /// \brief Returns the size of each block.
        std::size_t block_size() const noexcept
        {
            return list_.block_size();
        }

        /// \brief Returns the amount of memory left in the current block.
        std::size_t capacity() const noexcept
        {
            return cur_block_.memory_end(block_size()) - cur_mem_;
        }

        //=== misc ===//
        /// \brief Returns the implementation allocator.
        impl_allocator& get_impl_allocator() noexcept
        {
            return list_.get_allocator();
        }

    private:
        detail::memory_block_list<impl_allocator> list_;
        block_iter cur_block_;
        char *cur_mem_;
    };
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_STACK_ALLOCATOR_HPP_INCLUDED
