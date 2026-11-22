#ifndef YK_LZW20_DETAIL_ENCODER_TABLE_HPP
#define YK_LZW20_DETAIL_ENCODER_TABLE_HPP

#include <optional>
#include <vector>

#include <cstddef>
#include <cstdint>

namespace yk::lzw20::detail {

// Maps (prefix code, next byte) to the code of that string, by open addressing
// with linear probing. It starts small and grows, so that encoding a short input
// during constant evaluation does not pay for a table sized for 65536 codes.
class encoder_table {
public:
  [[nodiscard]] constexpr std::optional<std::uint32_t> find(std::uint32_t prefix, unsigned char byte) const
  {
    std::uint32_t const key = make_key(prefix, byte);
    for (std::size_t i = home(key);; i = next(i)) {
      if (slots_[i].key == empty) {
        return std::nullopt;
      }
      if (slots_[i].key == key) {
        return slots_[i].code;
      }
    }
  }

  // the pair must not be present yet
  constexpr void insert(std::uint32_t prefix, unsigned char byte, std::uint32_t code)
  {
    if ((size_ + 1) * 2 > slots_.size()) {
      grow();
    }
    place(make_key(prefix, byte), code);
    ++size_;
  }

  constexpr void clear() { *this = encoder_table(); }

private:
  struct slot {
    std::uint32_t key;
    std::uint32_t code;
  };

  // prefix < 2^16, so every key is below 2^24 and never equals empty
  static constexpr std::uint32_t empty = 0xFFFFFFFF;

  [[nodiscard]] static constexpr std::uint32_t make_key(std::uint32_t prefix, unsigned char byte) noexcept { return (prefix << 8) | byte; }

  [[nodiscard]] constexpr std::size_t home(std::uint32_t key) const noexcept
  {
    // Fibonacci hashing: the high bits of the product are well mixed
    return static_cast<std::size_t>((key * std::uint32_t{0x9E3779B9}) >> (32 - log2_capacity_));
  }

  [[nodiscard]] constexpr std::size_t next(std::size_t i) const noexcept { return (i + 1) & (slots_.size() - 1); }

  constexpr void place(std::uint32_t key, std::uint32_t code)
  {
    std::size_t i = home(key);
    while (slots_[i].key != empty) {
      i = next(i);
    }
    slots_[i] = slot{key, code};
  }

  constexpr void grow()
  {
    std::vector<slot> old(slots_.size() * 2, slot{empty, 0});
    old.swap(slots_);
    ++log2_capacity_;
    for (slot const& s : old) {
      if (s.key != empty) {
        place(s.key, s.code);
      }
    }
  }

  static constexpr unsigned initial_log2_capacity = 6;

  std::vector<slot> slots_ = std::vector<slot>(std::size_t{1} << initial_log2_capacity, slot{empty, 0});
  unsigned log2_capacity_ = initial_log2_capacity;
  std::size_t size_ = 0;
};

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_ENCODER_TABLE_HPP
