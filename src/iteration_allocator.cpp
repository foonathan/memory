// Copyright (C) 2015-2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "iteration_allocator.hpp"

#include "debugging.hpp"

using namespace foonathan::memory;

void detail::iteration_allocator_leak_handler::operator()(std::ptrdiff_t amount)
{
    get_leak_handler()({FOONATHAN_MEMORY_LOG_PREFIX "::iteration_allocator", this}, amount);
}

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class foonathan::memory::iteration_allocator<2>;
template class foonathan::memory::allocator_traits<iteration_allocator<2>>;
template class foonathan::memory::composable_allocator_traits<iteration_allocator<2>>;
#endif
