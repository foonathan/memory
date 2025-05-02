// Copyright (C) 2015-2025 Jonathan Müller and foonathan/memory contributors
// SPDX-License-Identifier: Zlib

#include "iteration_allocator.hpp"

using namespace foonathan::memory;

#if FOONATHAN_MEMORY_EXTERN_TEMPLATE
template class foonathan::memory::iteration_allocator<2>;
template class foonathan::memory::allocator_traits<iteration_allocator<2>>;
template class foonathan::memory::composable_allocator_traits<iteration_allocator<2>>;
#endif
