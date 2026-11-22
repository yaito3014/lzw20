#ifndef YK_LZW20_DETAIL_TO_ARRAY_HPP
#define YK_LZW20_DETAIL_TO_ARRAY_HPP

#include <algorithm>
#include <array>

#include <cstddef>

namespace yk::lzw20::detail {

// A std::vector allocated during constant evaluation cannot outlive it, so the
// result is computed twice: once for its size, which becomes the array bound,
// and once for its elements
template<class Make>
[[nodiscard]] consteval auto to_array(Make make)
{
  constexpr std::size_t size = make().size();
  using value_type = typename decltype(make())::value_type;
  std::array<value_type, size> result{};
  std::ranges::copy(make(), result.begin());
  return result;
}

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_TO_ARRAY_HPP
