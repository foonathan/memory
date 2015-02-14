#include "block_list.hpp"

#include <cassert>
#include <new>

using namespace foonathan::memory;
using namespace detail;

struct block_list_impl::node
{
    node *prev;
    std::size_t size;
    
    node(node *prev, std::size_t size) noexcept
    : prev(prev), size(size) {}
};

std::size_t block_list_impl::push(void* &memory, std::size_t size) noexcept
{
    auto ptr = ::new(memory) node(head_, size);
    head_ = ptr;
    memory = static_cast<char*>(memory) + sizeof(node);
    ++size_;
    return sizeof(node);
}

block_info block_list_impl::pop() noexcept
{
    assert(size_ != 0 && "stack underflow");
    auto top = head_;
    head_ = top->prev;
    --size_;
    return {top, top->size};
}