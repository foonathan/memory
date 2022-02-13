#include <cstddef>

template<typename T, size_t alignment>
struct align_of
{
  static_assert(alignment == 0, "this should fail, the purpose of this is to generate a compile error on this type that contains the alignment value on this target");
};

template<typename T>
using get_align_of = align_of<T, alignof(T)>;

get_align_of<TEST_TYPE> dummy;
