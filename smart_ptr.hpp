// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED
#define FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED

/// \file
/// \brief Smart pointer deleter and creators using \ref concept::RawAllocator.

#include <memory>

#include "allocator_adapter.hpp"

namespace foonathan { namespace memory
{
	/// \brief A deleter class that calls the appropriate deallocate function.
    /// \details It stores an \ref allocator_reference. It does not call any destrucotrs.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deallocator : allocator_reference<RawAllocator>
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;
        
        /// \brief Creates it giving it the allocator used for deallocation.
        raw_allocator_deallocator(allocator_reference<raw_allocator> alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)) {}
        
        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls \ref allocator_traits::deallocate_node, but no destructors.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }
        
        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }
    };
    
    /// \brief Specialization of \ref raw_allocator_deallocator for arrays.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deallocator<Type[], RawAllocator> : allocator_reference<RawAllocator>
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;
        
        /// \brief Creates it giving it the allocator used for deallocation and the array size.
        raw_allocator_deallocator(allocator_reference<raw_allocator> alloc,
                            std::size_t size) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)),
          size_(size) {}
        
        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls \ref allocator_traits::deallocate_array, but no destructors.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            this->deallocate_array(pointer, size_, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }
        
        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }
        
        /// \brief Returns the array size.
        std::size_t array_size() const FOONATHAN_NOEXCEPT
        {
            return size_;
        }
        
    private:
        std::size_t size_;
    };
    
    /// \brief A deleter class that calls the appropriate destructors and deallocate function.
    /// \details It stores an \ref allocator_reference. It calls destructors.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deleter : allocator_reference<RawAllocator>
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;
        
        /// \brief Creates it giving it the allocator used for deallocation.
        raw_allocator_deleter(allocator_reference<raw_allocator> alloc) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)) {}
        
        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls the destructor and \ref allocator_traits::deallocate_node.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            pointer->~value_type();
            this->deallocate_node(pointer, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }
        
        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }
    };
    
    /// \brief Specialization of \ref raw_allocator_deleter for arrays.
    /// \ingroup memory
    template <typename Type, class RawAllocator>
    class raw_allocator_deleter<Type[], RawAllocator> : allocator_reference<RawAllocator>
    {
    public:
        using raw_allocator = RawAllocator;
        using value_type = Type;
        
        /// \brief Creates it giving it the allocator used for deallocation and the array size.
        raw_allocator_deleter(allocator_reference<raw_allocator> alloc,
                            std::size_t size) FOONATHAN_NOEXCEPT
        : allocator_reference<RawAllocator>(std::move(alloc)),
          size_(size) {}
        
        /// \brief Deallocates the memory via the stored allocator.
        /// \details It calls the destructors and \ref allocator_traits::deallocate_array.
        void operator()(value_type *pointer) FOONATHAN_NOEXCEPT
        {
            for (auto cur = pointer; cur != pointer + size_; ++cur)
                cur->~value_type();
            this->deallocate_array(pointer, size_, sizeof(value_type), FOONATHAN_ALIGNOF(value_type));
        }
        
        /// \brief Returns a reference to the stored allocator.
        auto get_allocator() const FOONATHAN_NOEXCEPT
        -> decltype(std::declval<allocator_reference<raw_allocator>>().get_allocator())
        {
            return this->allocator_reference<raw_allocator>::get_allocator();
        }
        
        /// \brief Returns the array size.
        std::size_t array_size() const FOONATHAN_NOEXCEPT
        {
            return size_;
        }
        
    private:
        std::size_t size_;
    };
    
    namespace detail
    {
    	template <typename T, class RawAllocator, typename ... Args>
        auto allocate_unique(allocator_reference<RawAllocator> alloc, Args&&... args)
        -> std::unique_ptr<T, raw_allocator_deleter<T, RawAllocator>>
        {
            using raw_ptr = std::unique_ptr<T, raw_allocator_deallocator<T, RawAllocator>>;
            
            auto memory = alloc.allocate_node(sizeof(T), FOONATHAN_ALIGNOF(T));
            // raw_ptr deallocates memory in case of constructor exception
            raw_ptr result(static_cast<T*>(memory), {alloc});
            // call constructor
            ::new(memory) T(std::forward<Args>(args)...);
            // pass ownership to return value using a deleter that calls destructor
            return {result.release(), {alloc}};
        }
        
        template <typename T, typename ... Args>
        void construct(std::true_type, T *cur, T *end, Args&&... args)
        {
            for (; cur != end; ++cur)
                ::new(static_cast<void*>(cur)) T(std::forward<Args>(args)...);
        }
        
        template <typename T, typename ... Args>
        void construct(std::false_type, T *begin, T *end, Args&&... args)
        {
            auto cur = begin;
            try
            {
                for (; cur != end; ++cur)
                    ::new(static_cast<void*>(cur)) T(std::forward<Args>(args)...);
            }  
            catch (...)
            {
                for (auto el = begin; el != cur; ++el)
                    el->~T();
                throw;
            }
        }
        
        template <typename T, class RawAllocator>
        auto allocate_array_unique(std::size_t size, allocator_reference<RawAllocator> alloc)
        -> std::unique_ptr<T[], raw_allocator_deleter<T[], RawAllocator>>
        {
            using raw_ptr = std::unique_ptr<T[], raw_allocator_deallocator<T[], RawAllocator>>;
            
            auto memory = alloc.allocate_array(size, sizeof(T), FOONATHAN_ALIGNOF(T));
            // raw_ptr deallocates memory in case of constructor exception
            raw_ptr result(static_cast<T*>(memory), {alloc, size});
            construct(std::integral_constant<bool, FOONATHAN_NOEXCEPT(T())>{},
                    result.get(), result.get() + size);
            // pass ownership to return value using a deleter that calls destructor
            return {result.release(), {alloc, size}};
        }
    } // namespace detail

    /// \brief Creates an object wrapped in a \c std::unique_ptr using a \ref concept::RawAllocator.
    /// \ingroup memory
    template <typename T, class RawAllocator, typename ... Args>
    auto raw_allocate_unique(RawAllocator &&alloc, Args&&... args)
    -> typename std::enable_if
    <
        !std::is_array<T>::value,
        std::unique_ptr<T, raw_allocator_deleter<T, typename std::decay<RawAllocator>::type>>
    >::type
    {
        return detail::allocate_unique<T>(make_allocator_reference(std::forward<RawAllocator>(alloc)),
                                        std::forward<Args>(args)...);
    }
    
    /// \brief Creates an array wrapped in a \c std::unique_ptr using a \ref concept::RawAllocator.
    /// \ingroup memory
    template <typename T, class RawAllocator>
    auto raw_allocate_unique(RawAllocator &&alloc, std::size_t size)
    -> typename std::enable_if
    <
        std::is_array<T>::value,
        std::unique_ptr<T, raw_allocator_deleter<T, typename std::decay<RawAllocator>::type>>
    >::type
    {
        return detail::allocate_array_unique<typename std::remove_extent<T>::type>
                    (size, make_allocator_reference(std::forward<RawAllocator>(alloc)));
    }
    
    /// \brief Creates an object wrapped in a \c std::shared_ptr using a \ref concept::RawAllocator.
    /// \ingroup memory
    template <typename T, class RawAllocator, typename ... Args>
    std::shared_ptr<T> raw_allocate_shared(RawAllocator &&alloc, Args&&... args)
    {
        return std::allocate_shared<T>(make_std_allocator<T>(std::forward<RawAllocator>(alloc)), std::forward<Args>(args)...);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED
