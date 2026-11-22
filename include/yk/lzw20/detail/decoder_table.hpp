#ifndef YK_LZW20_DETAIL_DECODER_TABLE_HPP
#define YK_LZW20_DETAIL_DECODER_TABLE_HPP

#include <yk/lzw20/byte.hpp>
#include <yk/lzw20/detail/format.hpp>

#include <vector>

#include <cstddef>
#include <cstdint>

namespace yk::lzw20::detail {

// The strings of the codes from first_free_code on, each stored as its prefix
// code and last byte; codes below first_free_code are the single bytes
class decoder_table {
public:
  [[nodiscard]] constexpr std::uint32_t next_code() const noexcept { return first_free_code + static_cast<std::uint32_t>(entries_.size()); }

  [[nodiscard]] constexpr unsigned char first_byte(std::uint32_t code) const
  {
    return code < first_free_code ? static_cast<unsigned char>(code) : entries_[code - first_free_code].first;
  }

  // adds the string of prefix followed by byte as next_code()
  constexpr void add(std::uint32_t prefix, unsigned char byte)
  {
    std::size_t const prefix_length = prefix < first_free_code ? 1 : entries_[prefix - first_free_code].length;
    entries_.push_back(entry{prefix, prefix_length + 1, first_byte(prefix), byte});
  }

  // appends the string of code, which must be below next_code()
  template<byte_like Byte>
  constexpr void append_to(std::vector<Byte>& out, std::uint32_t code) const
  {
    std::size_t const length = code < first_free_code ? 1 : entries_[code - first_free_code].length;
    out.resize(out.size() + length);
    // the string is linked from its end, so fill it backwards
    auto it = out.end();
    for (; code >= first_free_code; code = entries_[code - first_free_code].prefix) {
      *--it = from_uchar<Byte>(entries_[code - first_free_code].last);
    }
    *--it = from_uchar<Byte>(static_cast<unsigned char>(code));
  }

private:
  struct entry {
    std::uint32_t prefix;
    std::size_t length;
    unsigned char first;
    unsigned char last;
  };

  std::vector<entry> entries_;
};

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_DECODER_TABLE_HPP
