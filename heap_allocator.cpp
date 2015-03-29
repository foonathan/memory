#include "heap_allocator.hpp"

#include <cassert>
#include <cstdlib>
#include <new>

using namespace foonathan::memory;

void* heap_allocator::allocate_node(std::size_t size, std::size_t)
{
    while (true)
    {
        auto mem = std::malloc(size);
        if (mem)
            return mem;
        auto handler = std::get_new_handler();
        if (!handler)
            throw std::bad_alloc();
        handler();
    }
    assert(false);
}

void heap_allocator::deallocate_node(void *ptr,
                                     std::size_t size, std::size_t) noexcept
{
    std::free(ptr);
}
