// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#ifndef FOONATHAN_MEMORY_DETAIL_EBO_STORAGE_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_EBO_STORAGE_HPP_INCLUDED

#include "utility.hpp"
#include "../config.hpp"

namespace foonathan
{
    namespace memory
    {
        namespace detail
        {
            template <int Tag, typename T>
            class ebo_storage : T
            {
            protected:
                ebo_storage(const T& t) : T(t) {}

                ebo_storage(T&& t) noexcept(std::is_nothrow_move_constructible<T>::value)
                : T(detail::move(t))
                {
                }

                T& get() noexcept
                {
                    return *this;
                }

                const T& get() const noexcept
                {
                    return *this;
                }
            };
        } // namespace detail
    } // namespace memory
} // namespace foonathan

#endif // FOONATHAN_MEMORY_DETAIL_EBO_STORAGE_HPP_INCLUDED
