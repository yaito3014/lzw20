#ifndef YK_LZW20_BYTE_HPP
#define YK_LZW20_BYTE_HPP

#include <concepts>
#include <ranges>

#include <climits>
#include <cstddef>

namespace yk::lzw20 {

static_assert(CHAR_BIT == 8, "the LZW stream is a sequence of octets");

template<class T>
concept byte_like =
    std::same_as<T, char> || std::same_as<T, signed char> || std::same_as<T, unsigned char> || std::same_as<T, char8_t> || std::same_as<T, std::byte>;

template<class R>
concept byte_input_range = std::ranges::input_range<R> && byte_like<std::ranges::range_value_t<R>>;

namespace detail {

template<byte_like Byte>
[[nodiscard]] constexpr unsigned char to_uchar(Byte b) noexcept
{
  return static_cast<unsigned char>(b);
}

template<byte_like Byte>
[[nodiscard]] constexpr Byte from_uchar(unsigned char b) noexcept
{
  return static_cast<Byte>(b);
}

}  // namespace detail

}  // namespace yk::lzw20

#endif  // YK_LZW20_BYTE_HPP
