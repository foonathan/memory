#ifndef FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED
#define FOONATHAN_MEMORY_ALLOCATOR_ADAPTER_HPP_INCLUDED

/// \file
/// \brief Adapters to wrap \ref concept::RawAllocator objects.

#include <limits>
#include <new>
#include <utility>

#include "allocator_traits.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        template <class RawAllocator, bool Stateful>
        class allocator_storage
        {
        public:
            allocator_storage(RawAllocator &allocator) noexcept
            : alloc_(&allocator) {}
            
        protected:
            ~allocator_storage() = default;
            
            RawAllocator& get_allocator() const noexcept
            {
                return *alloc_;
            }
            
        private:
            RawAllocator *alloc_;
        };
        
        template <class RawAllocator>
        class allocator_storage<RawAllocator, false>
        {
        public:
            allocator_storage() noexcept = default;
            allocator_storage(const RawAllocator&) noexcept {}
            
        protected:
            ~allocator_storage() = default;
            
            RawAllocator get_allocator() const noexcept
            {
                return {};
            }
        };
    } // namespace detail
    
    /// \brief A \ref concept::RawAllocator storing a pointer to an allocator.
    ///
    /// All allocation requests are forwarded to the stored allocator via the \ref allocator_traits.
    /// \ingroup memory
    template <class RawAllocator, class Traits = allocator_traits<RawAllocator>>
    class raw_allocator_adapter
    : detail::allocator_storage<typename Traits::allocator_state,
                            Traits::is_stateful::value>
    {
        using storage = detail::allocator_storage<typename Traits::allocator_state,
                            Traits::is_stateful::value>;
    public:
        using raw_allocator = RawAllocator;
        using is_stateful = typename Traits::is_stateful;

        /// \brief Creates it giving it the \ref allocator_state.
        /// \detail For non-stateful allocators, there exists a default-constructor and a version taking const-ref.
        /// For stateful allocators it takes a non-const reference.<br>
        /// Only stateful allocators are stored, non-stateful default-constructed on the fly.
        using storage::storage;

        void* allocate_node(std::size_t size, std::size_t alignment)
        {
            return Traits::allocate_node(get_allocator(), size, alignment);
        }
        
        void* allocate_array(std::size_t count,
                             std::size_t size, std::size_t alignment)
        {
            return Traits::allocate_array(get_allocator(), count, size, alignment);
        }

        void deallocate_node(void *ptr, std::size_t size, std::size_t alignment) noexcept
        {
            Traits::deallocate_node(get_allocator(), ptr, size, alignment);
        }
        
        void deallocate_array(void *array, std::size_t count,
                              std::size_t size, std::size_t alignment) noexcept
        {
            Traits::deallocate_array(get_allocator(), array, count, size, alignment);
        }
        
        /// \brief Returns the \ref allocator_state.
        auto get_allocator() const noexcept
        -> decltype(this->storage::get_allocator())
        {
            return storage::get_allocator();
        }
        
        std::size_t max_node_size() const noexcept
        {
            return Traits::max_node_size(get_allocator());
        }
        
        std::size_t max_array_size() const noexcept
        {
            return Traits::max_array_size(get_allocator());
        }
        
        std::size_t max_alignment() const noexcept
        {
            return Traits::max_alignment(get_allocator());
        }
    };

    /// \brief Wraps a \ref concept::RawAllocator to create an \c std::allocator.
    ///
    /// It uses a \ref raw_allocator_adapter to store the allocator to allow copy constructing.<br>
    /// The underlying allocator is never moved, only the pointer to it.<br>
    /// It does not propagate on assignment, only on swap, to ensure that the allocator always stays with its memory.
    /// \ingroup memory
    template <typename T, class RawAllocator>
    class raw_allocator_allocator
    : raw_allocator_adapter<RawAllocator>
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
        
        using propagate_on_container_swap = std::true_type;

        template <typename U>
        struct rebind {using other = raw_allocator_allocator<U, RawAllocator>;};

        using impl_allocator = RawAllocator;

        //=== constructor ===//
        raw_allocator_allocator() = default;
        using raw_allocator_adapter<RawAllocator>::raw_allocator_adapter;

        template <typename U>
        raw_allocator_allocator(const raw_allocator_allocator<U, RawAllocator> &alloc) noexcept
        : raw_allocator_adapter<RawAllocator>(alloc.get_impl_allocator()) {}

        //=== allocation/deallocation ===//
        pointer allocate(size_type n, void * = nullptr)
        {
            void *mem = nullptr;
            if (n == 1)
                mem = this->allocate_node(sizeof(value_type), alignof(value_type));
            else
                mem = this->allocate_array(n, sizeof(value_type), alignof(value_type));
            return static_cast<pointer>(mem);
        }

        void deallocate(pointer p, size_type n) noexcept
        {
            if (n == 1)
                this->deallocate_node(p, sizeof(value_type), alignof(value_type));
            else
                this->deallocate_array(p, n, sizeof(value_type), alignof(value_type));
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
            return this->max_array_size() / sizeof(value_type);
        }

        auto get_impl_allocator() const noexcept
        -> decltype(this->get_allocator())
        {
            return this->get_allocator();
        }
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
