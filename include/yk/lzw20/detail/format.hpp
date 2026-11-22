#ifndef YK_LZW20_DETAIL_FORMAT_HPP
#define YK_LZW20_DETAIL_FORMAT_HPP

#include <yk/lzw20/options.hpp>

#include <algorithm>
#include <bit>

#include <cstdint>

namespace yk::lzw20::detail {

inline constexpr std::uint32_t first_free_code = 256;
inline constexpr unsigned min_width = 9;
inline constexpr unsigned max_width_limit = 16;

template<options Opts>
inline constexpr bool valid_options = min_width <= Opts.max_width && Opts.max_width <= max_width_limit;

template<options Opts>
inline constexpr std::uint32_t max_codes = std::uint32_t{1} << Opts.max_width;

// The width of a code is decided by the encoder's next code to assign at the
// time it writes the code (at most max_codes, once the dictionary is full)
template<options Opts>
[[nodiscard]] constexpr unsigned code_width(std::uint32_t next_code) noexcept
{
  return std::min(Opts.max_width, static_cast<unsigned>(std::bit_width(next_code)));
}

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_FORMAT_HPP
