#include "new_allocator.hpp"

using namespace foonathan::memory;

void* new_allocator::allocate_node(std::size_t size, std::size_t)
{
    return ::operator new(size);
}

void new_allocator::deallocate_node(void* node, std::size_t size, std::size_t) noexcept
{
    ::operator delete(node);
}
