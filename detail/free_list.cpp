#include "free_list.hpp"

#include <cstddef>
#include <functional>
#include <utility>

using namespace foonathan::memory;
using namespace detail;

namespace
{
    // pre: ptr
    char*& next(void* ptr) noexcept
    {
        return *static_cast<char**>(ptr);
    }

    // pre: mem, el_size > sizeof(mem), size >= el_size
    void* build_list(void* &mem, std::size_t el_size, std::size_t size) noexcept
    {
        auto ptr = static_cast<char*>(mem),
             end = static_cast<char*>(mem) + size - el_size;
        for (; ptr != end; ptr += el_size)
            next(ptr) = ptr + el_size;
        return ptr;
    }

    // pre: list, mem
    std::pair<void*, void*> find_position(void *list, void *mem) noexcept
    {
        auto greater = std::greater<char*>();
        auto prev = static_cast<char*>(list);
        if (greater(prev, static_cast<char*>(mem)))
            return std::make_pair(nullptr, list);

        auto ptr = next(prev);
        while (ptr)
        {
            if (greater(ptr, static_cast<char*>(mem)))
                break;
            prev = ptr;
            ptr = next(ptr);
        }

        return std::make_pair(prev, ptr);
    }

    // pre: beg
    bool check_n(char* &cur, std::size_t n, std::size_t el_size) noexcept
    {
        --n; // we already have one (cur)
        for (; cur; cur += el_size)
        {
            if (next(cur) == cur + el_size)
            {
                if (--n == 0)
                    break;
            }
            else
                return false;
        }
        return true;
    }
}

free_memory_list::free_memory_list(std::size_t el_size,
                                   void *mem, std::size_t size) noexcept
: first_(nullptr), el_size_(el_size)
{
    insert(mem, size);
}

void free_memory_list::insert(void *mem, std::size_t size) noexcept
{
    auto last = build_list(mem, el_size_, size);
    next(last) = first_;
    first_ = static_cast<char*>(mem);
}

void free_memory_list::insert_ordered(void *mem, std::size_t size) noexcept
{
    if (empty())
        return insert(mem, size);

    auto pos = find_position(first_, mem);
    insert_between(pos.first, pos.second,
                   mem, size);
}

void free_memory_list::insert_between(void *pre, void *after,
                                      void *mem, std::size_t size) noexcept
{
    auto last = build_list(mem, el_size_, size);

    if (pre)
        next(pre) = static_cast<char*>(mem);
    else
        first_ = static_cast<char*>(mem);

    next(last) = static_cast<char*>(after);
}

void* free_memory_list::allocate() noexcept
{
    auto block = first_;
    first_ = next(first_);
    return block;
}

void* free_memory_list::allocate(std::size_t n) noexcept
{
    for(auto cur = first_; cur; cur = next(cur))
    {
        auto start = cur;
        if (check_n(cur, n, el_size_))
        {
            first_ = next(start);
            return start;
        }
    }
    return nullptr;
}

void free_memory_list::deallocate(void *ptr) noexcept
{
    next(ptr) = first_;
    first_ = static_cast<char*>(ptr);
}

void free_memory_list::deallocate_ordered(void *ptr) noexcept
{
    auto pos = find_position(first_, ptr);
    insert_between(pos.first, pos.second, ptr, el_size_);
}
