#include "new_allocator.hpp"

#include "tracking.hpp"

using namespace foonathan::memory;

void* new_allocator::allocate_node(std::size_t size, std::size_t)
{
    auto mem = ::operator new(size);
    detail::on_heap_alloc(true, mem, size);
    return mem;
}

void new_allocator::deallocate_node(void* node, std::size_t size, std::size_t) noexcept
{
    detail::on_heap_alloc(false, node, size);
    ::operator delete(node);
}
