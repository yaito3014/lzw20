#ifndef YK_LZW20_DETAIL_BIT_IO_HPP
#define YK_LZW20_DETAIL_BIT_IO_HPP

#include <yk/lzw20/byte.hpp>
#include <yk/lzw20/options.hpp>

#include <iterator>
#include <optional>
#include <utility>
#include <vector>

#include <cstdint>

namespace yk::lzw20::detail {

// Codes are at most 16 bits wide and at most 7 bits stay pending between
// calls, so the 32-bit buffers below never overflow

[[nodiscard]] constexpr std::uint32_t low_bits(unsigned count) noexcept { return (std::uint32_t{1} << count) - 1; }

template<bit_order Order, byte_like Byte>
class bit_writer {
public:
  constexpr void write(std::uint32_t code, unsigned width)
  {
    if constexpr (Order == bit_order::lsb_first) {
      buffer_ |= code << count_;
      count_ += width;
      for (; count_ >= 8; count_ -= 8) {
        out_.push_back(from_uchar<Byte>(static_cast<unsigned char>(buffer_ & 0xFF)));
        buffer_ >>= 8;
      }
    } else {
      buffer_ = (buffer_ << width) | code;
      count_ += width;
      for (; count_ >= 8; count_ -= 8) {
        out_.push_back(from_uchar<Byte>(static_cast<unsigned char>((buffer_ >> (count_ - 8)) & 0xFF)));
      }
      buffer_ &= low_bits(count_);
    }
  }

  // pads the last byte with zero bits
  [[nodiscard]] constexpr std::vector<Byte> finish() &&
  {
    if (count_ > 0) {
      std::uint32_t const last = Order == bit_order::lsb_first ? buffer_ : buffer_ << (8 - count_);
      out_.push_back(from_uchar<Byte>(static_cast<unsigned char>(last & 0xFF)));
    }
    return std::move(out_);
  }

private:
  std::vector<Byte> out_;
  std::uint32_t buffer_ = 0;
  unsigned count_ = 0;
};

template<bit_order Order, std::input_iterator I, std::sentinel_for<I> S>
class bit_reader {
public:
  constexpr bit_reader(I first, S last) : it_(std::move(first)), last_(std::move(last)) {}

  // nullopt when fewer than width bits remain: they are the padding of the last byte
  [[nodiscard]] constexpr std::optional<std::uint32_t> read(unsigned width)
  {
    for (; count_ < width && it_ != last_; ++it_) {
      std::uint32_t const b = to_uchar(*it_);
      if constexpr (Order == bit_order::lsb_first) {
        buffer_ |= b << count_;
      } else {
        buffer_ = (buffer_ << 8) | b;
      }
      count_ += 8;
    }
    if (count_ < width) {
      return std::nullopt;
    }
    count_ -= width;
    if constexpr (Order == bit_order::lsb_first) {
      std::uint32_t const code = buffer_ & low_bits(width);
      buffer_ >>= width;
      return code;
    } else {
      std::uint32_t const code = buffer_ >> count_;
      buffer_ &= low_bits(count_);
      return code;
    }
  }

private:
  I it_;
  S last_;
  std::uint32_t buffer_ = 0;
  unsigned count_ = 0;
};

}  // namespace yk::lzw20::detail

#endif  // YK_LZW20_DETAIL_BIT_IO_HPP
