#ifndef YK_LZW20_DECODE_HPP
#define YK_LZW20_DECODE_HPP

#include <yk/lzw20/detail/bit_io.hpp>
#include <yk/lzw20/detail/decoder_table.hpp>
#include <yk/lzw20/detail/format.hpp>
#include <yk/lzw20/detail/to_array.hpp>

#include <yk/lzw20/byte.hpp>
#include <yk/lzw20/options.hpp>

#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include <cstdint>

namespace yk::lzw20 {

class decode_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// Throws decode_error on a code the encoder cannot have written. During
// constant evaluation the throw makes the expression non-constant, so malformed
// input becomes a compile error.
template<options Opts = options{}, byte_like Byte = unsigned char, byte_input_range R>
[[nodiscard]] constexpr std::vector<Byte> decode(R&& input)
{
  static_assert(detail::valid_options<Opts>, "max_width must be in [9, 16]");

  std::vector<Byte> out;
  detail::bit_reader<Opts.order, std::ranges::iterator_t<R>, std::ranges::sentinel_t<R>> reader(std::ranges::begin(input), std::ranges::end(input));
  detail::decoder_table table;

  std::optional<std::uint32_t> const first = reader.read(detail::code_width<Opts>(detail::first_free_code));
  if (!first) {
    return out;
  }
  if (*first >= detail::first_free_code) {
    throw decode_error("yk::lzw20::decode: the first code is not a single byte");
  }
  table.append_to(out, *first);

  for (std::uint32_t prev = *first;;) {
    // the encoder assigns a code after each one it writes, so it was one code ahead
    std::uint32_t const encoder_next = std::min(table.next_code() + 1, detail::max_codes<Opts>);
    std::optional<std::uint32_t> const code = reader.read(detail::code_width<Opts>(encoder_next));
    if (!code) {
      return out;
    }
    bool const can_add = table.next_code() < detail::max_codes<Opts>;
    if (*code < table.next_code()) {
      if (can_add) {
        table.add(prev, table.first_byte(*code));
      }
    } else if (*code == table.next_code() && can_add) {
      // the string of prev followed by its own first byte, which the encoder
      // assigned just before writing it
      table.add(prev, table.first_byte(prev));
    } else {
      throw decode_error("yk::lzw20::decode: a code is not in the dictionary yet");
    }
    table.append_to(out, *code);
    prev = *code;
  }
}

template<byte_like Byte, options Opts = options{}, byte_input_range R>
[[nodiscard]] constexpr std::vector<Byte> decode(R&& input)
{
  return lzw20::decode<Opts, Byte>(std::forward<R>(input));
}

// The input is passed as a function returning it, because a function parameter
// is not a constant expression, e.g. decode_array([] { return encoded; })
template<options Opts = options{}, byte_like Byte = unsigned char, class F>
  requires std::invocable<F const&> && byte_input_range<std::invoke_result_t<F const&>>
[[nodiscard]] consteval auto decode_array(F input)
{
  return detail::to_array([input] { return lzw20::decode<Opts, Byte>(input()); });
}

template<byte_like Byte, options Opts = options{}, class F>
  requires std::invocable<F const&> && byte_input_range<std::invoke_result_t<F const&>>
[[nodiscard]] consteval auto decode_array(F input)
{
  return lzw20::decode_array<Opts, Byte>(input);
}

}  // namespace yk::lzw20

#endif  // YK_LZW20_DECODE_HPP
