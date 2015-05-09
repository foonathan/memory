// Copyright (C) 2015 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED

#include <cstddef>
#include <cstdint>

#include "../config.hpp"

namespace foonathan { namespace memory
{
	namespace detail
    {
        // returns the offset needed to align ptr for given alignment
        // alignment must be a power of two
    	inline std::size_t align_offset(void *ptr, std::size_t alignment) FOONATHAN_NOEXCEPT
        {
            auto address = reinterpret_cast<std::uintptr_t>(ptr);
            auto misaligned = address & (alignment - 1);
            // misaligned != 0 ? (alignment - misaligned) : 0
            return misaligned * (alignment - misaligned);
        }
        
        // max_align_t is sometimes not in namespace std and sometimes not available at all
        #if FOONATHAN_IMPL_HAS_MAX_ALIGN
            namespace max_align
            {
                using namespace std;
                using type = max_align_t;
            }
            
            FOONATHAN_CONSTEXPR auto max_alignment = FOONATHAN_ALIGNOF(max_align::type);
        #else
            // assume long double has maximum alignment
            FOONATHAN_CONSTEXPR auto max_alignment = FOONATHAN_ALIGNOF(long double);
        #endif
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
