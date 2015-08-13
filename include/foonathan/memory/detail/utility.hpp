// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_UTILITY_HPP
#define FOONATHAN_MEMORY_DETAIL_UTILITY_HPP

// implementation of some functions from <utility> to prevent dependencies on it

#include <type_traits>

#include "../config.hpp"

#if FOONATHAN_HOSTED_IMPLEMENTATION
    #include <utility>
#endif

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
            // sometimes std::swap is found via ADL
            // use it instead of swap_impl::swap to prevent ambiguity
            // when both functions exist, i.e. on hosted
        #if FOONATHAN_HOSTED_IMPLEMENTATION
            using std::swap;
        #else
            using swap_impl::swap;
        #endif
            swap(a, b);
        }

        // fancier syntax for enable_if
        // used as (template) parameter
        // also useful for doxygen
        // define PREDEFINED: FOONATHAN_REQUIRES(x):=
        #define FOONATHAN_REQUIRES(Expr) \
            typename std::enable_if<(Expr), int>::type = 0

        // same as above, but as return type
        // also useful for doxygen:
        // defined PREDEFINED: FOONATHAN_REQUIRES_RET(x,r):=r
        #define FOONATHAN_REQUIRES_RET(Expr, ...) \
            typename std::enable_if<(Expr), __VA_ARGS__>::type

        // fancier syntax for general expression SFINAE
        // used as (template) parameter
        // also useful for doxygen:
        // define PREDEFINED: FOONATHAN_SFINAE(x):=
        #define FOONATHAN_SFINAE(Expr) \
            decltype((Expr), int()) = 0

        // avoids code repetition for one-line forwarding functions
        #define FOONATHAN_AUTO_RETURN(Expr) \
            decltype(Expr) {return Expr;}

        // same as above, but requires certain type
        #define FOONATHAN_AUTO_RETURN_TYPE(Expr, T) \
            decltype(Expr) \
            { \
                static_assert(std::is_same<decltype(Expr), T>::value, "wrong return type"); \
                return Expr; \
            }
    } // namespace detail
}} // namespace foonathan::memory

#endif //FOONATHAN_MEMORY_DETAIL_UTILITY_HPP
