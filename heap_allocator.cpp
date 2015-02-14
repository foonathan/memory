#include "heap_allocator.hpp"

#include <cassert>
#include <cstdlib>
#include <new>

#include "tracking.hpp"

using namespace foonathan::memory;

void* heap_allocator::allocate_node(std::size_t size, std::size_t)
{
    while (true)
    {
        auto mem = std::malloc(size);
        if (mem)
        {
            detail::on_heap_alloc(true, mem, size);
            return mem;
        }
        auto handler = std::get_new_handler();
        if (!handler)
            throw std::bad_alloc();
        handler();
    }
    assert(false);
}

void* heap_allocator::allocate_array(std::size_t count, std::size_t size, std::size_t alignment)
{
    return allocate_node(count * size, alignment);
}

void heap_allocator::deallocate_node(void *ptr,
                                     std::size_t size, std::size_t) noexcept
{
    std::free(ptr);
    detail::on_heap_alloc(false, ptr, size);
}

void heap_allocator::deallocate_array(void *ptr, std::size_t count,
                                      std::size_t size, std::size_t alignment) noexcept
{
    deallocate_node(ptr, count * size, alignment);
}
