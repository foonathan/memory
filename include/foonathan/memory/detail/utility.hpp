// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_UTILITY_HPP
#define FOONATHAN_MEMORY_DETAIL_UTILITY_HPP

// implementation of some functions from <utility> to prevent dependencies on it

#include <type_traits>

#include "../config.hpp"

namespace foonathan { namespace memory
{
    namespace detail
    {
        // move - taken from http://stackoverflow.com/a/7518365
        template <typename T>
        typename std::remove_reference<T>::type&& move(T&& arg) FOONATHAN_NOEXCEPT
        {
            return static_cast<typename std::remove_reference<T>::type&&>(arg);
        }

        // forward - taken from http://stackoverflow.com/a/27501759
        template <class T>
        T&& forward(typename std::remove_reference<T>::type& t) FOONATHAN_NOEXCEPT
        {
            return static_cast<T&&>(t);
        }

        template <class T>
        T&& forward(typename std::remove_reference<T>::type&& t) FOONATHAN_NOEXCEPT
        {
            static_assert(!std::is_lvalue_reference<T>::value,
                          "Can not forward an rvalue as an lvalue.");
            return static_cast<T&&>(t);
        }

        namespace swap_impl
        {
            // equivalent of std::swap but requires nothrow
            template <typename T>
            void swap(T &a, T &b) FOONATHAN_NOEXCEPT
            {
                T tmp = detail::move(a);
                a = detail::move(b);
                b = detail::move(tmp);
            }
        } // namespace swap_impl

        // ADL aware swap
        template <typename T>
        void adl_swap(T &a, T &b) FOONATHAN_NOEXCEPT
        {
            using swap_impl::swap;
            swap(a, b);
        }
    } // namespace detail
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_DETAIL_UTILITY_HPP
