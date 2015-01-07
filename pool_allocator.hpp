#ifndef FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED
#define FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED

/// \file
/// \brief A pool allocator.

#include <cmath>
#include "pool.hpp"
#include "std_allocator_base.hpp"

namespace foonathan { namespace memory
{
    /// \cond impl
    namespace impl
    {
        template <class Impl>
        void* pool_allocate(std::true_type, memory_pool<Impl> &pool, std::size_t size)
        {
            auto n = std::ceil(float(size) / pool.element_size());
            return pool.allocate_array_ordered(n);
        }

        template <class Impl>
        void* pool_allocate(std::false_type, memory_pool<Impl> &pool, std::size_t size)
        {
           assert(size <= pool.element_size() && "requested size for non-array bigger than pool allocators element size");
            return pool.allocate();
        }

        template <class Impl>
        void pool_deallocate(std::true_type, memory_pool<Impl> &pool,
                             void *ptr, std::size_t size) noexcept
        {
            auto n = std::ceil(float(size) / pool.element_size());
            pool.deallocate_array_ordered(ptr, n);
        }

        template <class Impl>
        void pool_deallocate(std::false_type, memory_pool<Impl> &pool,
                             void *ptr, std::size_t) noexcept
        {
            pool.deallocate(ptr);
        }

        template <bool AllocatesArray, class ImplRawAllocator>
        class pool_allocator_base
        {
        public:
            using allocates_array = std::integral_constant<bool, AllocatesArray>;
            using impl_allocator = ImplRawAllocator;
            using pool = memory_pool<impl_allocator>;

            explicit pool_allocator_base(pool &p) noexcept
            : pool_(&p) {}

            //=== getter ===//
            /// \brief Returns the underlying pool.
            pool& get_pool() const noexcept
            {
                return *pool_;
            }

        protected:
            ~pool_allocator_base() noexcept = default;

        private:
            pool *pool_;
        };
    } // namespace impl
    /// \endcond

    /// \brief A \ref concept::RawAllocator using a \ref memory_pool.
    ///
    /// It holds a reference to the memory pool. <br>
    /// As a pool allocator can only allocate a fixed element size,
    /// there is a problem with rebinding when using a \ref raw_allocator_allocator.
    /// You need to check your stdlib-implementation to find out the actual element size
    /// that will be allocated (e.g. the size of a \c std::list-node).<br>
    /// Some containers allocate arrays (e.g. \c std::deque implementations),
    /// in this case, the allocator needs to behave differently - set the \c AllocatesArray parameter accordingly.
    /// The block size of the \c memory pool must be big enough to hold the allocated array size.
    /// Because of this, you may ran into problems when using big \c std::vectors,
    /// then you might choose a different allocator.
    /// \ingroup memory
    template <bool AllocatesArray = false,
              class ImplRawAllocator = heap_allocator>
    class pool_raw_allocator : public detail::pool_allocator_base<AllocatesArray,
                                        ImplRawAllocator>
    {
        using pool_alloc = detail::pool_allocator_base<AllocatesArray, ImplRawAllocator>;
    public:
        using stateful =  std::true_type;

        using pool_alloc::pool_allocator_base;
        pool_raw_allocator(const pool_raw_allocator&) = delete;
        pool_raw_allocator(pool_raw_allocator&&) = default;

        //=== allocation/deallocation ===//
        void* allocate(std::size_t size, std::size_t)
        {
            return detail::pool_allocate(typename pool_alloc::allocates_array(), this->get_pool(), size);
        }

        void deallocate(void *ptr, std::size_t size, std::size_t) noexcept
        {
            detail::pool_deallocate(typename pool_alloc::allocates_array(), this->get_pool(), ptr, size);
        }
    };

    /// \brief An \c Allocator using a \ref memory_pool.
    ///
    /// It is simple a more efficient wrapper for \ref pool_raw_allocator than \ref raw_allocator_allocator.
    /// \ingroup memory
    template <typename T,
              bool AllocatesArray = false,
              class ImplRawAllocator = heap_allocator>
    class pool_allocator : public detail::pool_allocator_base<AllocatesArray,
                                                            ImplRawAllocator>,
                           public std_allocator_base<pool_allocator<T, AllocatesArray,
                                                                    ImplRawAllocator>,
                                                     T>
    {
        using std_alloc = std_allocator_base<pool_allocator<T, AllocatesArray,
                                                                    ImplRawAllocator>, T>;
        using pool_alloc = detail::pool_allocator_base<AllocatesArray, ImplRawAllocator>;

    public:
        template <typename U>
        struct rebind
        {
            using other = pool_allocator<U, AllocatesArray, ImplRawAllocator>;
        };

        using pool_alloc::pool_allocator_base;

        template <typename U>
        pool_allocator(const pool_allocator<U, AllocatesArray, ImplRawAllocator> &other) noexcept
        : pool_alloc(other.get_pool()) {}

        //=== allocation/deallocation ===//
        typename std_alloc::pointer allocate(typename std_alloc::size_type n)
        {
            assert(AllocatesArray || n == 1 && "you said you did not want to allocate arrays");
            n *= sizeof(T);
            auto mem = detail::pool_allocate(typename pool_alloc::allocates_array(), this->get_pool(), n);
            return static_cast<T*>(mem);
        }

        void deallocate(typename std_alloc::pointer ptr, typename std_alloc::size_type n)  noexcept
        {
            n *= sizeof(T);
            detail::pool_deallocate(typename pool_alloc::allocates_array(), this->get_pool(), ptr, n);
        }
    };

    template <typename T, bool Arr, class Impl, typename U>
    bool operator==(const pool_allocator<T, Arr, Impl> &lhs,
                    const pool_allocator<U, Arr, Impl> &rhs) noexcept
    {
        return &lhs.get_pool() == &rhs.get_pool();
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_POOL_ALLOCATOR_HPP_INCLUDED
