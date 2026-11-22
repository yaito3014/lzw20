#ifndef YK_LZW20_ENCODE_HPP
#define YK_LZW20_ENCODE_HPP

#include <yk/lzw20/detail/bit_io.hpp>
#include <yk/lzw20/detail/encoder_table.hpp>
#include <yk/lzw20/detail/format.hpp>
#include <yk/lzw20/detail/to_array.hpp>

#include <yk/lzw20/byte.hpp>
#include <yk/lzw20/options.hpp>

#include <algorithm>
#include <concepts>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <cstdint>

namespace yk::lzw20 {

namespace detail {

template<options Opts, byte_like Byte>
[[nodiscard]] constexpr unsigned char to_literal(Byte b)
{
  unsigned char const literal = to_uchar(b);
  if (literal >= literal_count<Opts>) {
    throw std::invalid_argument("yk::lzw20::encode: a byte is not below 2^alphabet_width");
  }
  return literal;
}

}  // namespace detail

// Throws std::invalid_argument on a byte that is not a literal. During constant
// evaluation the throw makes the expression non-constant, so such input becomes
// a compile error.
template<options Opts = options{}, byte_like Byte = unsigned char, byte_input_range R>
[[nodiscard]] constexpr std::vector<Byte> encode(R&& input)
{
  static_assert(
      detail::valid_options<Opts>,
      "alphabet_width must be in [1, 8] (at least 7 without control codes), max_width at most 16 with room for a dictionary code, "
      "and full_table::clear needs control_codes::clear_eoi"
  );
  constexpr bool has_control_codes = Opts.control == control_codes::clear_eoi;

  detail::bit_writer<Opts.order, Byte> writer;
  std::uint32_t next_code = detail::first_free_code<Opts>;
  if constexpr (has_control_codes) {
    writer.write(detail::clear_code<Opts>, detail::code_width<Opts>(next_code));
  }
  auto it = std::ranges::begin(input);
  auto const last = std::ranges::end(input);
  if (it == last) {
    if constexpr (has_control_codes) {
      writer.write(detail::eoi_code<Opts>, detail::code_width<Opts>(next_code));
    }
    return std::move(writer).finish();
  }

  detail::encoder_table table;
  std::uint32_t prefix = detail::to_literal<Opts>(*it);
  for (++it; it != last; ++it) {
    unsigned char const byte = detail::to_literal<Opts>(*it);
    if (auto const code = table.find(prefix, byte)) {
      prefix = *code;
      continue;
    }
    writer.write(prefix, detail::code_width<Opts>(next_code));
    if (next_code < detail::max_codes<Opts>) {
      table.insert(prefix, byte, next_code);
      ++next_code;
    } else if constexpr (Opts.on_full == full_table::clear) {
      writer.write(detail::clear_code<Opts>, detail::code_width<Opts>(next_code));
      table.clear();
      next_code = detail::first_free_code<Opts>;
    }
    prefix = byte;
  }
  writer.write(prefix, detail::code_width<Opts>(next_code));
  if constexpr (has_control_codes) {
    // the decoder reads every code after a data code as if the encoder had assigned one more code
    writer.write(detail::eoi_code<Opts>, detail::code_width<Opts>(std::min(next_code + 1, detail::max_codes<Opts>)));
  }
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
