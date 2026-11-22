#ifndef YK_LZW20_OPTIONS_HPP
#define YK_LZW20_OPTIONS_HPP

namespace yk::lzw20 {

enum class bit_order {
  lsb_first,  // codes fill each byte from the least significant bit (GIF, Unix compress)
  msb_first,  // codes fill each byte from the most significant bit (TIFF, PDF)
};

struct options {
  // the code width starts at 9 bits and grows up to max_width (9..16), where the dictionary freezes
  unsigned max_width = 12;
  bit_order order = bit_order::lsb_first;
};

}  // namespace yk::lzw20

#endif  // YK_LZW20_OPTIONS_HPP
