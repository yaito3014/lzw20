#include <yk/lzw20.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <random>
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

namespace {

template<lzw::options Opts>
void check_round_trips()
{
  std::mt19937 rng(12345);
  for (std::size_t const size : std::initializer_list<std::size_t>{1, 2, 3, 100, 4096, 100'000}) {
    // a small alphabet makes long matches and fills the dictionary
    for (unsigned const alphabet : {2u, 4u, 256u}) {
      std::uniform_int_distribution<unsigned> dist(0, alphabet - 1);
      bytes input(size);
      std::ranges::generate(input, [&] { return static_cast<unsigned char>(dist(rng)); });
      CAPTURE(Opts.max_width, Opts.order == lzw::bit_order::msb_first, size, alphabet);
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
