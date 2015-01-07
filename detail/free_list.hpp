#ifndef FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED

#include <cstddef>

namespace foonathan { namespace memory
{
    /// \cond impl
    namespace detail
    {
        class free_memory_list
        {
        public:
            //=== constructor ===//
            // does not own memory!
            free_memory_list(std::size_t el_size,
                             void *mem, std::size_t size) noexcept;

            //=== insert/allocation/deallocation ===//
            // does not own memory!
            void insert(void *mem, std::size_t size) noexcept;
            void insert_ordered(void *mem, std::size_t size) noexcept;

            // pre: !empty()
            void* allocate() noexcept;

            // pre: !empty()
            // can return nullptr if no block found
            // won't necessarily work if non-ordered functions are called
            void* allocate(std::size_t n) noexcept;

            void deallocate(void *ptr) noexcept;
            void deallocate(void *ptr, std::size_t n) noexcept
            {
                insert(ptr, n * element_size());
            }

            void deallocate_ordered(void *ptr) noexcept;
            void deallocate_ordered(void *ptr, std::size_t n) noexcept
            {
                insert_ordered(ptr, n * element_size());
            }

            //=== getter ===//
            std::size_t element_size() const noexcept
            {
                return el_size_;
            }

            bool empty() const noexcept
            {
                return !first_;
            }

        private:
            void insert_between(void *pre, void *after,
                                void *mem, std::size_t size) noexcept;

            char *first_;
            std::size_t el_size_;
        };
    } // namespace detail
    /// \endcond
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAILL_FREE_LIST_HPP_INCLUDED
