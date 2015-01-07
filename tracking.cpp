#include "tracking.hpp"

#include <atomic>

using namespace foonathan::memory;

namespace
{
    void default_allocation_tracker(bool, void*, std::size_t) noexcept {}
    void default_growth_tracker(const char*, void*, std::size_t) noexcept {}

    std::atomic<heap_allocation_tracker> alloc_tracker(default_allocation_tracker);
    std::atomic<allocator_growth_tracker> growth_tracker(default_growth_tracker);
}

heap_allocation_tracker foonathan::memory::set_heap_allocation_tracker(heap_allocation_tracker t) noexcept
{
    return alloc_tracker.exchange(t, std::memory_order_relaxed);
}

allocator_growth_tracker foonathan::memory::set_allocator_growth_tracker(allocator_growth_tracker t) noexcept
{
    return growth_tracker.exchange(t, std::memory_order_relaxed);
}

void foonathan::memory::detail::on_heap_alloc(bool allocation, void *ptr, std::size_t size) noexcept
{
    heap_allocation_tracker t = alloc_tracker;
    t(allocation, ptr, size);
}

void foonathan::memory::detail::on_allocator_growth(const char *name, void *ptr, std::size_t size)
{
    allocator_growth_tracker t = growth_tracker;
    t(name, ptr, size);
}
