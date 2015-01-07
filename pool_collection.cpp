#include "pool_collection.hpp"

#include <cassert>
#include <climits>

using namespace foonathan::memory;

namespace
{
    constexpr bool is_power_of_two(std::size_t no) noexcept
    {
        return no && (no & (no - 1)) == 0;
    }

    std::size_t get_index(std::size_t size) noexcept
    {
        auto base_power = sizeof(size) * CHAR_BIT - __builtin_clzl(size);
        return base_power - (is_power_of_two(size) ? 1 : 0);
    }
}

memory_pool_collection::memory_pool_collection(std::size_t no_elements_pool,
                                               std::size_t no_pools)
: pools_(*this), no_elements_pool_(no_elements_pool)
{
    pools_.reserve(no_pools);
}

void* memory_pool_collection::allocate(std::size_t size, std::size_t)
{
    auto index = get_index(size);
    auto el_size = 1 << index;
    if (PORTAL_UNLIKELY(!has_pool(index, el_size)))
    {
        detail::on_allocator_growth("portal::memory_pool_collection",
                                  this, el_size);
        insert_pool(index, el_size, no_elements_pool_);
    }
    return pools_[index].allocate();
}

void memory_pool_collection::deallocate(void *mem, std::size_t size, std::size_t) noexcept
{
    auto index = get_index(size);
    pools_[index].deallocate(mem);
}

void memory_pool_collection::insert_pool(std::size_t el_size, std::size_t no_elements)
{
    assert(is_power_of_two(el_size) && !has_pool(el_size) && "invalid element size");
    insert_pool(get_index(el_size), el_size, no_elements);
}

bool memory_pool_collection::has_pool(std::size_t el_size) const noexcept
{
    auto index = get_index(el_size);
    return has_pool(index, el_size);
}

memory_pool<>& memory_pool_collection::get_pool(std::size_t el_size) noexcept
{
    auto index = get_index(el_size);
    assert(has_pool(index, el_size) && "cannot access non-existent pool");
    return pools_[index];
}

void memory_pool_collection::insert_pool(std::size_t index, std::size_t el_size, std::size_t no_el)
{
    auto iter = pools_.begin() + index;
    pools_.emplace(iter, el_size, el_size * no_el);
}

bool memory_pool_collection::has_pool(std::size_t index, std::size_t el_size) const noexcept
{
    return index < pools_.size() && pools_[index].element_size() == el_size;
}
