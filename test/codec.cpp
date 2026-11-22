#include <yk/lzw20.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <random>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

using bytes = std::vector<unsigned char>;

using namespace std::string_view_literals;

namespace lzw = yk::lzw20;

constexpr lzw::options lsb{.max_width = 12, .order = lzw::bit_order::lsb_first};
constexpr lzw::options msb{.max_width = 12, .order = lzw::bit_order::msb_first};

bytes to_bytes(std::string_view s) { return bytes(s.begin(), s.end()); }

// expected values below are from tools/gen_reference.py

std::uint64_t fnv1a(bytes const& data)
{
  std::uint64_t h = 0xCBF29CE484222325;
  for (unsigned char const b : data) {
    h = (h ^ b) * 0x100000001B3;
  }
  return h;
}

bytes growth_input()
{
  bytes data;
  for (std::size_t i = 0; i < 600; ++i) {
    data.push_back(static_cast<unsigned char>((i * 7 + (i / 256) * 13) % 256));
  }
  return data;
}

bytes quaternary_input(std::size_t size)
{
  bytes data;
  for (std::size_t i = 0; i < size; ++i) {
    data.push_back(static_cast<unsigned char>((i * i + i / 3) % 4));
  }
  return data;
}

// the variant used by GIF, for a GIF with a minimum code size of Alphabet
template<unsigned Alphabet, unsigned MaxWidth = 12, lzw::full_table OnFull = lzw::full_table::freeze, lzw::bit_order Order = lzw::bit_order::lsb_first>
constexpr lzw::options gif{
    .max_width = MaxWidth,
    .order = Order,
    .alphabet_width = Alphabet,
    .control = lzw::control_codes::clear_eoi,
    .growth = lzw::code_growth::last_code,
    .on_full = OnFull,
};

}  // namespace

TEST_CASE("empty input", "[codec]")
{
  CHECK(lzw::encode(""sv).empty());
  CHECK(lzw::decode(bytes{}).empty());
  CHECK(lzw::encode<msb>(""sv).empty());
  CHECK(lzw::decode<msb>(bytes{}).empty());
}

TEST_CASE("single byte", "[codec]")
{
  CHECK(lzw::encode<lsb>("A"sv) == bytes{0x41, 0x00});
  CHECK(lzw::encode<msb>("A"sv) == bytes{0x20, 0x80});
  CHECK(lzw::decode<lsb>(bytes{0x41, 0x00}) == to_bytes("A"));
  CHECK(lzw::decode<msb>(bytes{0x20, 0x80}) == to_bytes("A"));
}

TEST_CASE("TOBEORNOT", "[codec]")
{
  // codes 84 79 66 69 79 82 78 79 84 256 258 260 265 259 261 263, all 9 bits wide
  auto const input = "TOBEORNOTTOBEORTOBEORNOT"sv;
  bytes const expected_lsb{0x54, 0x9E, 0x08, 0x29, 0xF2, 0x44, 0x8A, 0x93, 0x27, 0x54, 0x00, 0x0A, 0x24, 0x98, 0x70, 0x60, 0xC1, 0x83};
  bytes const expected_msb{0x2A, 0x13, 0xC8, 0x44, 0x52, 0x79, 0x48, 0x9C, 0x4F, 0x2A, 0x40, 0x20, 0x50, 0x48, 0x4C, 0x0E, 0x0B, 0x07};

  CHECK(lzw::encode(input) == expected_lsb);
  CHECK(lzw::encode<lsb>(input) == expected_lsb);
  CHECK(lzw::encode<msb>(input) == expected_msb);
  CHECK(lzw::decode(expected_lsb) == to_bytes(input));
  CHECK(lzw::decode<msb>(expected_msb) == to_bytes(input));
}

TEST_CASE("code not yet in the decoder's dictionary (KwKwK)", "[codec]")
{
  // codes 65 256 257 258: each refers to the entry the decoder is about to add
  auto const input = "AAAAAAAAAA"sv;
  bytes const expected_lsb{0x41, 0x00, 0x06, 0x14, 0x08};
  bytes const expected_msb{0x20, 0xC0, 0x20, 0x30, 0x20};

  CHECK(lzw::encode<lsb>(input) == expected_lsb);
  CHECK(lzw::encode<msb>(input) == expected_msb);
  CHECK(lzw::decode<lsb>(expected_lsb) == to_bytes(input));
  CHECK(lzw::decode<msb>(expected_msb) == to_bytes(input));
}

TEST_CASE("code width grows from 9 to 10 bits", "[codec]")
{
  // 429 codes, so the width steps up once code 512 is the next to assign
  bytes const input = growth_input();

  bytes const lsb_out = lzw::encode<lsb>(input);
  CHECK(lsb_out.size() == 505);
  CHECK(fnv1a(lsb_out) == 0x07789AC922A4CED1);
  CHECK(lzw::decode<lsb>(lsb_out) == input);

  bytes const msb_out = lzw::encode<msb>(input);
  CHECK(msb_out.size() == 505);
  CHECK(fnv1a(msb_out) == 0x36D1B37F52685018);
  CHECK(lzw::decode<msb>(msb_out) == input);
}

TEST_CASE("dictionary freezes at max_width", "[codec]")
{
  constexpr lzw::options lsb9{.max_width = 9, .order = lzw::bit_order::lsb_first};
  constexpr lzw::options msb9{.max_width = 9, .order = lzw::bit_order::msb_first};
  bytes const input = growth_input();

  bytes const lsb_out = lzw::encode<lsb9>(input);
  CHECK(lsb_out.size() == 483);
  CHECK(fnv1a(lsb_out) == 0x421794F583347205);
  CHECK(lzw::decode<lsb9>(lsb_out) == input);

  bytes const msb_out = lzw::encode<msb9>(input);
  CHECK(msb_out.size() == 483);
  CHECK(fnv1a(msb_out) == 0x0370CEBDCB8224B1);
  CHECK(lzw::decode<msb9>(msb_out) == input);
}

TEST_CASE("GIF variant: empty input", "[codec][gif]")
{
  // CLEAR, EOI
  CHECK(lzw::encode<gif<2>>(""sv) == bytes{0x2C});
  CHECK(lzw::encode<gif<2, 12, lzw::full_table::freeze, lzw::bit_order::msb_first>>(""sv) == bytes{0x94});
  CHECK(lzw::decode<gif<2>>(bytes{0x2C}).empty());
  CHECK(lzw::decode<gif<2, 12, lzw::full_table::freeze, lzw::bit_order::msb_first>>(bytes{0x94}).empty());
}

TEST_CASE("GIF variant: 2-bit alphabet", "[codec][gif]")
{
  // codes 4 0 1 0 2 1 2 2 3 2 0 3 0 6 8 10 12 14 16 18 9 11 13 15 17 7 25 21 28 24 20 27 23 30 5,
  // 3 to 6 bits wide, each width growing once the last code assigned no longer fits
  bytes const input = quaternary_input(64);
  bytes const expected_lsb{0x44, 0x20, 0x21, 0x32, 0x02, 0x03, 0x0C, 0x52, 0xCC, 0x41, 0x99, 0x56, 0x7B, 0xF1, 0xE4, 0xCA, 0x61, 0xD4, 0x76, 0x79, 0x05};
  bytes const expected_msb{0x80, 0x82, 0x12, 0x23, 0x20, 0x30, 0x19, 0x0A, 0x63, 0xA1, 0x24, 0xAD, 0xAF, 0x89, 0xF3, 0x57, 0x18, 0x51, 0xB5, 0xDE, 0x14};
  constexpr auto msb_gif = gif<2, 12, lzw::full_table::freeze, lzw::bit_order::msb_first>;

  CHECK(lzw::encode<gif<2>>(input) == expected_lsb);
  CHECK(lzw::encode<msb_gif>(input) == expected_msb);
  CHECK(lzw::decode<gif<2>>(expected_lsb) == input);
  CHECK(lzw::decode<msb_gif>(expected_msb) == input);
}

TEST_CASE("GIF variant: same stream as Pillow writes", "[codec][gif]")
{
  // the image data of quaternary_input(64) as an 8x8 GIF written by Pillow 13.0.0, which uses a minimum code size of 8
  bytes const input = quaternary_input(64);
  bytes const pillow{0x00, 0x01, 0x04, 0x00, 0x20, 0x20, 0x80, 0x00, 0x01, 0x03, 0x04, 0x00, 0x18, 0x00, 0x40, 0x20, 0x41, 0x83, 0x08, 0x15,
                     0x32, 0x74, 0x58, 0xF0, 0x60, 0xC2, 0x85, 0x0D, 0x07, 0x56, 0x8C, 0x88, 0x91, 0x22, 0xC4, 0x8B, 0x13, 0x35, 0x06, 0x04};

  CHECK(lzw::encode<gif<8>>(input) == pillow);
  CHECK(lzw::decode<gif<8>>(pillow) == input);
}

TEST_CASE("GIF variant: code width grows from 9 to 10 bits", "[codec][gif]")
{
  // 431 codes with CLEAR and EOI
  bytes const input = growth_input();

  bytes const lsb_out = lzw::encode<gif<8>>(input);
  CHECK(lsb_out.size() == 507);
  CHECK(fnv1a(lsb_out) == 0xDE478E1669CF4706);
  CHECK(lzw::decode<gif<8>>(lsb_out) == input);

  constexpr auto msb_gif = gif<8, 12, lzw::full_table::freeze, lzw::bit_order::msb_first>;
  bytes const msb_out = lzw::encode<msb_gif>(input);
  CHECK(msb_out.size() == 507);
  CHECK(fnv1a(msb_out) == 0x3CDB088E4D254A10);
  CHECK(lzw::decode<msb_gif>(msb_out) == input);
}

TEST_CASE("GIF variant: full dictionary", "[codec][gif]")
{
  bytes const input = quaternary_input(300);

  SECTION("freeze")
  {
    // 182 codes, the dictionary frozen at 16 codes
    bytes const out = lzw::encode<gif<2, 4>>(input);
    CHECK(out.size() == 91);
    CHECK(fnv1a(out) == 0x7DF4FF05417FB18C);
    CHECK(lzw::decode<gif<2, 4>>(out) == input);
  }

  SECTION("clear")
  {
    // 329 codes, 28 of them CLEAR: one each time the dictionary fills
    constexpr auto clearing = gif<2, 4, lzw::full_table::clear>;
    bytes const out = lzw::encode<clearing>(input);
    CHECK(out.size() == 154);
    CHECK(fnv1a(out) == 0x904B4C695547D26B);
    CHECK(lzw::decode<clearing>(out) == input);
    // the decoder does not need to know which policy the encoder used
    CHECK(lzw::decode<gif<2, 4>>(out) == input);
  }
}

TEST_CASE("GIF variant: decoding", "[codec][gif]")
{
  // CLEAR 0 1 CLEAR 1 0 EOI, all 3 bits wide: a CLEAR may come before the dictionary is full
  CHECK(lzw::decode<gif<2>>(bytes{0x44, 0x18, 0x14}) == bytes{0, 1, 1, 0});
  // the input after EOI is not read
  CHECK(lzw::decode<gif<2>>(bytes{0x44, 0x18, 0x14, 0xFF, 0xFF}) == bytes{0, 1, 1, 0});
  // a stream need not start with CLEAR: 0 1 EOI
  CHECK(lzw::decode<gif<2>>(bytes{0x48, 0x01}) == bytes{0, 1});
}

TEST_CASE("GIF variant: malformed input", "[codec][gif]")
{
  // CLEAR 0, then the input ends without EOI
  CHECK_THROWS_AS(lzw::decode<gif<2>>(bytes{0x04}), lzw::decode_error);
  CHECK_THROWS_AS(lzw::decode<gif<2>>(bytes{}), lzw::decode_error);
  // CLEAR 6: the first code after CLEAR is not a literal
  CHECK_THROWS_AS(lzw::decode<gif<2>>(bytes{0x34}), lzw::decode_error);
  // CLEAR 0 7: the decoder's next code to assign is 6, so 7 cannot be in the dictionary
  CHECK_THROWS_AS(lzw::decode<gif<2>>(bytes{0xC4, 0x01}), lzw::decode_error);
  // 4 does not fit in a 2-bit alphabet
  CHECK_THROWS_AS(lzw::encode<gif<2>>(bytes{0, 4}), std::invalid_argument);
}

namespace {

template<lzw::options Opts>
void check_round_trips()
{
  std::mt19937 rng(12345);
  for (std::size_t const size : std::initializer_list<std::size_t>{1, 2, 3, 100, 4096, 100'000}) {
    // a small alphabet makes long matches and fills the dictionary
    for (unsigned const alphabet : {2u, 4u, 256u}) {
      std::uniform_int_distribution<unsigned> dist(0, std::min(alphabet, 1u << Opts.alphabet_width) - 1);
      bytes input(size);
      std::ranges::generate(input, [&] { return static_cast<unsigned char>(dist(rng)); });
      CAPTURE(
          Opts.max_width, Opts.order == lzw::bit_order::msb_first, Opts.alphabet_width, Opts.control == lzw::control_codes::clear_eoi,
          Opts.growth == lzw::code_growth::last_code, Opts.on_full == lzw::full_table::clear, size, alphabet
      );
      CHECK(lzw::decode<Opts>(lzw::encode<Opts>(input)) == input);
    }
  }
}

}  // namespace

TEST_CASE("round trip", "[codec]")
{
  check_round_trips<lzw::options{.max_width = 9, .order = lzw::bit_order::lsb_first}>();
  check_round_trips<lzw::options{.max_width = 9, .order = lzw::bit_order::msb_first}>();
  check_round_trips<lzw::options{.max_width = 12, .order = lzw::bit_order::lsb_first}>();
  check_round_trips<lzw::options{.max_width = 12, .order = lzw::bit_order::msb_first}>();
  check_round_trips<lzw::options{.max_width = 16, .order = lzw::bit_order::lsb_first}>();
  check_round_trips<lzw::options{.max_width = 16, .order = lzw::bit_order::msb_first}>();
}

TEST_CASE("round trip with the GIF variant", "[codec][gif]")
{
  check_round_trips<gif<2>>();
  check_round_trips<gif<2, 3, lzw::full_table::clear>>();
  check_round_trips<gif<2, 12, lzw::full_table::clear, lzw::bit_order::msb_first>>();
  check_round_trips<gif<4, 5>>();
  check_round_trips<gif<8>>();
  check_round_trips<gif<8, 12, lzw::full_table::clear>>();
  check_round_trips<gif<8, 16, lzw::full_table::clear>>();
}

TEST_CASE("round trip with each option on its own", "[codec]")
{
  check_round_trips<lzw::options{.alphabet_width = 7}>();
  check_round_trips<lzw::options{.max_width = 8, .alphabet_width = 7}>();
  check_round_trips<lzw::options{.alphabet_width = 1, .control = lzw::control_codes::clear_eoi}>();
  check_round_trips<lzw::options{.max_width = 3, .alphabet_width = 1, .control = lzw::control_codes::clear_eoi}>();
  check_round_trips<lzw::options{.control = lzw::control_codes::clear_eoi}>();
  check_round_trips<lzw::options{.control = lzw::control_codes::clear_eoi, .on_full = lzw::full_table::clear}>();
  check_round_trips<lzw::options{.max_width = 9, .growth = lzw::code_growth::last_code}>();
}

TEST_CASE("byte types", "[codec]")
{
  auto const text = "TOBEORNOTTOBEORTOBEORNOT"sv;
  bytes const expected = lzw::encode(text);

  SECTION("input")
  {
    CHECK(lzw::encode(std::vector<char>(text.begin(), text.end())) == expected);
    CHECK(lzw::encode(std::vector<signed char>(text.begin(), text.end())) == expected);
    CHECK(lzw::encode(std::vector<char8_t>(text.begin(), text.end())) == expected);
    std::vector<std::byte> as_bytes;
    for (char const c : text) {
      as_bytes.push_back(static_cast<std::byte>(c));
    }
    CHECK(lzw::encode(as_bytes) == expected);
  }

  SECTION("output")
  {
    std::vector<std::byte> const encoded = lzw::encode<std::byte>(text);
    REQUIRE(encoded.size() == expected.size());
    CHECK(std::ranges::equal(encoded, expected, {}, [](std::byte b) { return std::to_integer<unsigned char>(b); }));

    std::vector<char> const decoded = lzw::decode<char>(encoded);
    CHECK(std::string_view(decoded.data(), decoded.size()) == text);

    CHECK(lzw::encode<lsb, std::byte>(text) == encoded);
    CHECK(lzw::decode<msb, char>(lzw::encode<msb>(text)) == std::vector<char>(text.begin(), text.end()));
  }
}

TEST_CASE("malformed input", "[codec]")
{
  // first code 300: not a literal
  CHECK_THROWS_AS(lzw::decode(bytes{0x2C, 0x01}), lzw::decode_error);
  // 65, then 258 while the next code to assign is 256
  CHECK_THROWS_AS(lzw::decode(bytes{0x41, 0x04, 0x02}), lzw::decode_error);
  // a trailing partial code is padding, not an error
  CHECK(lzw::decode(bytes{0x41}) == bytes{});
}
