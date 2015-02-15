#ifndef FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED
#define FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED

namespace foonathan { namespace memory
{
	namespace detail
    {
        // returns the offset needed to align ptr for given alignment
        // alignment must be a power of two
    	std::size_t align_offset(void *ptr, std::size_t alignment) noexcept
        {
            auto address = reinterpret_cast<std::uintptr_t>(ptr);
            auto misaligned = address & (alignment - 1);
            // misaligned != 0 ? (alignment - misaligned) : 0
            return misaligned * (alignment - misaligned);
        }
    } // namespace detail
}} // namespace foonathan::memory

#endif // FOONATHAN_MEMORY_DETAIL_ALIGN_HPP_INCLUDED