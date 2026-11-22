#ifndef YK_LZW20_OPTIONS_HPP
#define YK_LZW20_OPTIONS_HPP

namespace yk::lzw20 {

enum class bit_order {
  lsb_first,  // codes fill each byte from the least significant bit (GIF, Unix compress)
  msb_first,  // codes fill each byte from the most significant bit (TIFF, PDF)
};

enum class control_codes {
  none,       // every code after the literals is a dictionary entry
  clear_eoi,  // the two codes after the literals are CLEAR, which empties the dictionary, and EOI, which ends the stream (GIF)
};

enum class code_growth {
  next_code,  // the code width fits the next code to assign
  last_code,  // the code width fits the last code assigned, so it grows one code later (GIF)
};

enum class full_table {
  freeze,  // the encoder keeps using the full dictionary
  clear,   // the encoder writes CLEAR and starts over; needs control_codes::clear_eoi. The decoder accepts either.
};

struct options {
  // the code width starts at alphabet_width + 1 bits or more and grows up to max_width (at most 16), where the dictionary is full
  unsigned max_width = 12;
  bit_order order = bit_order::lsb_first;
  // the literals are the codes below 2^alphabet_width, in [1, 8] with control_codes::clear_eoi and in [7, 8] without,
  // since then the stream ends where the input does; every input byte of the encoder must be a literal
  unsigned alphabet_width = 8;
  control_codes control = control_codes::none;
  code_growth growth = code_growth::next_code;
  full_table on_full = full_table::freeze;
};

}  // namespace yk::lzw20

#endif  // YK_LZW20_OPTIONS_HPP
