// Copyright (C) 2015-2021 Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include "iteration_allocator.hpp"

using namespace foonathan::memory;

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class foonathan::memory::iteration_allocator<2>;
template class foonathan::memory::allocator_traits<iteration_allocator<2>>;
template class foonathan::memory::composable_allocator_traits<iteration_allocator<2>>;
#endif
