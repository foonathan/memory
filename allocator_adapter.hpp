#ifndef FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED

/// \file
/// \brief Adapters to wrap \ref concept::RawAllocator objects.

#include <limits>
#include <new>

namespace foonathan { namespace memory
{
    /// \brief A \ref concept::RawAllocator storing a pointer to an allocator.
    ///
    /// All allocation requests are forwarded to the stored allocator.
    /// \ingroup memory
    template <class RawAllocator>
    class raw_allocator_adapter
    {
    public:
        using raw_allocator = RawAllocator;

        raw_allocator_adapter(raw_allocator &alloc) noexcept
        : alloc_(&alloc) {}

        void* allocate(std::size_t size, std::size_t alignment)
        {
            return alloc_->allocate(size, alignment);
        }

        void deallocate(void *ptr, std::size_t size, std::size_t alignment) noexcept
        {
            alloc_->deallocate(ptr, size, alignment);
        }

        raw_allocator& get_allocator() const noexcept
        {
            return *alloc_;
        }

    private:
        raw_allocator *alloc_;
    };

    /// \brief Wraps a \ref concept::RawAllocator to create an \c std::allocator.
    ///
    /// Be careful, allocators are freely copied or not copied in most library implementations.
    /// And furthermore, the standard requires that a copy of an allocator can deallocate all memory allocated with it. <br>
    /// Because of this, the stored object is a \ref raw_allocator_adapter
    /// and you are only allowed to interchange containers that are referencing to the same allocator!
    /// \ingroup memory
    template <typename T, class RawAllocator>
    class raw_allocator_allocator
    {
    public:
        //=== typedefs ===//
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        template <typename U>
        struct rebind {using other = raw_allocator_allocator<U, RawAllocator>;};

        using impl_allocator = RawAllocator;

        //=== constructor ===//
        raw_allocator_allocator(impl_allocator &alloc) noexcept
        : alloc_(alloc) {}

        template <typename U>
        raw_allocator_allocator(const raw_allocator_allocator<U, RawAllocator> &alloc) noexcept
        : alloc_(alloc.get_impl_allocator()) {}

        //=== allocation/deallocation ===//
        pointer allocate(size_type n, void * = nullptr)
        {
            auto mem = alloc_.allocate(n * sizeof(value_type), alignof(value_type));
            return static_cast<pointer>(mem);
        }

        void deallocate(pointer p, size_type n) noexcept
        {
            alloc_.deallocate(p, n * sizeof(value_type), alignof(value_type));
        }

        //=== construction/destruction ===//
        template <typename U, typename ... Args>
        void construct(U *p, Args&&... args)
        {
            void* mem = p;
            ::new(mem) U(std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U *p) noexcept
        {
            p->~U();
        }

        //=== getter ===//
        size_type max_size() const noexcept
        {
            return std::numeric_limits<size_type>::max();
        }

        impl_allocator& get_impl_allocator() const noexcept
        {
            return alloc_.get_allocator();
        }

    private:
        raw_allocator_adapter<impl_allocator> alloc_;
    };

    template <typename T, typename U, class Impl>
    bool operator==(const raw_allocator_allocator<T, Impl> &lhs,
                    const raw_allocator_allocator<U, Impl> &rhs) noexcept
    {
        return &lhs.get_impl_allocator() == &rhs.get_impl_allocator();
    }

    template <typename T, typename U, class Impl>
    bool operator!=(const raw_allocator_allocator<T, Impl> &lhs,
                    const raw_allocator_allocator<U, Impl> &rhs) noexcept
    {
        return !(lhs == rhs);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
