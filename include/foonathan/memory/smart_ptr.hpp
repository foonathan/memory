// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED
#define FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED

/// \file
/// \brief Smart pointer creators using \ref concept::RawAllocator.
/// \note Only available on a hosted implementation.

#include "config.hpp"
#if !FOONATHAN_HOSTED_IMPLEMENTATION
    #error "This header is only available for a hosted implementation."
#endif

#include <memory>

#include "deleter.hpp"

namespace foonathan { namespace memory
{
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
            ::new(memory) T(detail::forward<Args>(args)...);
            // pass ownership to return value using a deleter that calls destructor
            return {result.release(), {alloc}};
        }

        template <typename T, typename ... Args>
        void construct(std::true_type, T *cur, T *end, Args&&... args)
        {
            for (; cur != end; ++cur)
                ::new(static_cast<void*>(cur)) T(detail::forward<Args>(args)...);
        }

        template <typename T, typename ... Args>
        void construct(std::false_type, T *begin, T *end, Args&&... args)
        {
        #if FOONATHAN_HAS_EXCEPTION_SUPPORT
            auto cur = begin;
            try
            {
                for (; cur != end; ++cur)
                    ::new(static_cast<void*>(cur)) T(detail::forward<Args>(args)...);
            }
            catch (...)
            {
                for (auto el = begin; el != cur; ++el)
                    el->~T();
                throw;
            }
        #else
            construct(std::true_type{}, begin, end, detail::forward<Args>(args)...);
        #endif
        }

        template <typename T, class RawAllocator>
        auto allocate_array_unique(std::size_t size, allocator_reference<RawAllocator> alloc)
        -> std::unique_ptr<T[], raw_allocator_deleter<T[], RawAllocator>>
        {
            using raw_ptr = std::unique_ptr<T[], raw_allocator_deallocator<T[], RawAllocator>>;

            auto memory = alloc.allocate_array(size, sizeof(T), FOONATHAN_ALIGNOF(T));
            // raw_ptr deallocates memory in case of constructor exception
            raw_ptr result(static_cast<T*>(memory), {alloc, size});
            construct(std::integral_constant<bool, FOONATHAN_NOEXCEPT_OP(T())>{},
                    result.get(), result.get() + size);
            // pass ownership to return value using a deleter that calls destructor
            return {result.release(), {alloc, size}};
        }
    } // namespace detail

    /// \brief Creates an object wrapped in a \c std::unique_ptr using a \ref concept::RawAllocator.
    /// \ingroup memory
    template <typename T, class RawAllocator, typename ... Args>
    auto raw_allocate_unique(RawAllocator &&alloc, Args&&... args)
    -> FOONATHAN_REQUIRES_RET(!std::is_array<T>::value,
                        std::unique_ptr<T, raw_allocator_deleter<T, typename std::decay<RawAllocator>::type>>)
    {
        return detail::allocate_unique<T>(make_allocator_reference(detail::forward<RawAllocator>(alloc)),
                                        detail::forward<Args>(args)...);
    }

    /// \brief Creates an array wrapped in a \c std::unique_ptr using a \ref concept::RawAllocator.
    /// \ingroup memory
    template <typename T, class RawAllocator>
    auto raw_allocate_unique(RawAllocator &&alloc, std::size_t size)
    -> FOONATHAN_REQUIRES_RET(std::is_array<T>::value,
                              std::unique_ptr<T, raw_allocator_deleter<T, typename std::decay<RawAllocator>::type>>)
    {
        return detail::allocate_array_unique<typename std::remove_extent<T>::type>
                    (size, make_allocator_reference(detail::forward<RawAllocator>(alloc)));
    }

    /// \brief Creates an object wrapped in a \c std::shared_ptr using a \ref concept::RawAllocator.
    /// \ingroup memory
    template <typename T, class RawAllocator, typename ... Args>
    std::shared_ptr<T> raw_allocate_shared(RawAllocator &&alloc, Args&&... args)
    {
        return std::allocate_shared<T>(make_std_allocator<T>(detail::forward<RawAllocator>(alloc)), detail::forward<Args>(args)...);
    }
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_SMART_PTR_HPP_INCLUDED
