#ifndef YK_LZW20_ENCODE_HPP
#define YK_LZW20_ENCODE_HPP

#include <yk/lzw20/byte.hpp>
#include <yk/lzw20/detail/bit_io.hpp>
#include <yk/lzw20/detail/encoder_table.hpp>
#include <yk/lzw20/detail/format.hpp>
#include <yk/lzw20/detail/to_array.hpp>
#include <yk/lzw20/options.hpp>

#include <concepts>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include <cstdint>

namespace yk::lzw20 {

template<options Opts = options{}, byte_like Byte = unsigned char, byte_input_range R>
[[nodiscard]] constexpr std::vector<Byte> encode(R&& input)
{
  static_assert(detail::valid_options<Opts>, "max_width must be in [9, 16]");

  detail::bit_writer<Opts.order, Byte> writer;
  auto it = std::ranges::begin(input);
  auto const last = std::ranges::end(input);
  if (it == last) {
    return std::move(writer).finish();
  }

  detail::encoder_table table;
  std::uint32_t next_code = detail::first_free_code;
  std::uint32_t prefix = detail::to_uchar(*it);
  for (++it; it != last; ++it) {
    unsigned char const byte = detail::to_uchar(*it);
    if (auto const code = table.find(prefix, byte)) {
      prefix = *code;
      continue;
    }
    writer.write(prefix, detail::code_width<Opts>(next_code));
    if (next_code < detail::max_codes<Opts>) {
      table.insert(prefix, byte, next_code);
      ++next_code;
    }
    prefix = byte;
  }
  writer.write(prefix, detail::code_width<Opts>(next_code));
  return std::move(writer).finish();
}

template<byte_like Byte, options Opts = options{}, byte_input_range R>
[[nodiscard]] constexpr std::vector<Byte> encode(R&& input)
{
  return lzw20::encode<Opts, Byte>(std::forward<R>(input));
}

// The input is passed as a function returning it, because a function parameter
// is not a constant expression, e.g. encode_array([] { return "abc"sv; })
template<options Opts = options{}, byte_like Byte = unsigned char, class F>
  requires std::invocable<F const&> && byte_input_range<std::invoke_result_t<F const&>>
[[nodiscard]] consteval auto encode_array(F input)
{
  return detail::to_array([input] { return lzw20::encode<Opts, Byte>(input()); });
}

template<byte_like Byte, options Opts = options{}, class F>
  requires std::invocable<F const&> && byte_input_range<std::invoke_result_t<F const&>>
[[nodiscard]] consteval auto encode_array(F input)
{
  return lzw20::encode_array<Opts, Byte>(input);
}

}  // namespace yk::lzw20

#endif  // YK_LZW20_ENCODE_HPP
