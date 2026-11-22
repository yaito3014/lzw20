#ifndef YK_LZW20_DETAIL_DECODER_TABLE_HPP
#define YK_LZW20_DETAIL_DECODER_TABLE_HPP

#include <yk/lzw20/byte.hpp>

#include <vector>

#include <cstddef>
#include <cstdint>

namespace yk::lzw20::detail {

// The strings of the codes from first_free_code on, each stored as its prefix
// code and last byte; the codes passed in below first_free_code must be
// literals, which are the single bytes
class decoder_table {
public:
  constexpr explicit decoder_table(std::uint32_t first_free_code) noexcept : first_free_code_(first_free_code) {}

  [[nodiscard]] constexpr std::uint32_t next_code() const noexcept { return first_free_code_ + static_cast<std::uint32_t>(entries_.size()); }

  constexpr void clear() noexcept { entries_.clear(); }

  [[nodiscard]] constexpr unsigned char first_byte(std::uint32_t code) const
  {
    return code < first_free_code_ ? static_cast<unsigned char>(code) : entries_[code - first_free_code_].first;
  }

  // adds the string of prefix followed by byte as next_code()
  constexpr void add(std::uint32_t prefix, unsigned char byte)
  {
    std::size_t const prefix_length = prefix < first_free_code_ ? 1 : entries_[prefix - first_free_code_].length;
    entries_.push_back(entry{prefix, prefix_length + 1, first_byte(prefix), byte});
  }

  // appends the string of code, which must be below next_code()
  template<byte_like Byte>
  constexpr void append_to(std::vector<Byte>& out, std::uint32_t code) const
  {
    std::size_t const length = code < first_free_code_ ? 1 : entries_[code - first_free_code_].length;
    out.resize(out.size() + length);
    // the string is linked from its end, so fill it backwards
    auto it = out.end();
    for (; code >= first_free_code_; code = entries_[code - first_free_code_].prefix) {
      *--it = from_uchar<Byte>(entries_[code - first_free_code_].last);
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

  std::uint32_t first_free_code_;
  std::vector<entry> entries_;
};

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_DECODER_TABLE_HPP
