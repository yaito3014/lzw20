#ifndef YK_LZW20_DETAIL_FORMAT_HPP
#define YK_LZW20_DETAIL_FORMAT_HPP

#include <yk/lzw20/options.hpp>

#include <algorithm>
#include <bit>

#include <cstdint>

namespace yk::lzw20::detail {

inline constexpr unsigned max_alphabet_width = 8;
inline constexpr unsigned max_width_limit = 16;

// Without EOI the stream ends where the input does, so the at most 7 bits
// padding the last byte must not hold a whole code: codes need 8 bits or more
inline constexpr unsigned min_alphabet_width_without_eoi = 7;

// checked in this order so that no shift below is out of range
template<options Opts>
inline constexpr bool valid_options =
    1 <= Opts.alphabet_width && Opts.alphabet_width <= max_alphabet_width && Opts.max_width <= max_width_limit
    && (std::uint32_t{1} << Opts.alphabet_width) + (Opts.control == control_codes::clear_eoi ? 2 : 0) < (std::uint32_t{1} << Opts.max_width)
    && (Opts.control == control_codes::clear_eoi || Opts.alphabet_width >= min_alphabet_width_without_eoi)
    && (Opts.on_full == full_table::freeze || Opts.control == control_codes::clear_eoi);

template<options Opts>
inline constexpr std::uint32_t literal_count = std::uint32_t{1} << Opts.alphabet_width;

template<options Opts>
inline constexpr std::uint32_t clear_code = literal_count<Opts>;

template<options Opts>
inline constexpr std::uint32_t eoi_code = literal_count<Opts> + 1;

template<options Opts>
inline constexpr std::uint32_t first_free_code = literal_count<Opts> + (Opts.control == control_codes::clear_eoi ? 2 : 0);

template<options Opts>
inline constexpr std::uint32_t max_codes = std::uint32_t{1} << Opts.max_width;

// The width of a code is decided by the encoder's next code to assign at the
// time it writes the code (at most max_codes, once the dictionary is full)
template<options Opts>
[[nodiscard]] constexpr unsigned code_width(std::uint32_t next_code) noexcept
{
  // next_code is at least first_free_code, which is at least 2
  std::uint32_t const fitting = Opts.growth == code_growth::next_code ? next_code : next_code - 1;
  return std::clamp(static_cast<unsigned>(std::bit_width(fitting)), Opts.alphabet_width + 1, Opts.max_width);
}

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_FORMAT_HPP
