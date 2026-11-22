#include <yk/lzw20.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

using namespace std::string_view_literals;

namespace lzw = yk::lzw20;

constexpr lzw::options msb{.max_width = 12, .order = lzw::bit_order::msb_first};

constexpr auto tobe = "TOBEORNOTTOBEORTOBEORNOT"sv;

// from tools/gen_reference.py
constexpr std::array<unsigned char, 18> tobe_lsb{0x54, 0x9E, 0x08, 0x29, 0xF2, 0x44, 0x8A, 0x93, 0x27, 0x54, 0x00, 0x0A, 0x24, 0x98, 0x70, 0x60, 0xC1, 0x83};
constexpr std::array<unsigned char, 18> tobe_msb{0x2A, 0x13, 0xC8, 0x44, 0x52, 0x79, 0x48, 0x9C, 0x4F, 0x2A, 0x40, 0x20, 0x50, 0x48, 0x4C, 0x0E, 0x0B, 0x07};

constexpr auto tobe_encoded = lzw::encode_array([] { return tobe; });
static_assert(std::is_same_v<decltype(tobe_encoded), std::array<unsigned char, 18> const>);
static_assert(tobe_encoded == tobe_lsb);
static_assert(lzw::encode_array<msb>([] { return tobe; }) == tobe_msb);

constexpr auto tobe_decoded = lzw::decode_array([] { return tobe_lsb; });
static_assert(std::is_same_v<decltype(tobe_decoded), std::array<unsigned char, tobe.size()> const>);
static_assert(std::ranges::equal(tobe_decoded, tobe, {}, [](char c) { return static_cast<unsigned char>(c); }));
static_assert(std::ranges::equal(lzw::decode_array<msb>([] { return tobe_msb; }), tobe_decoded));

// the caller picks the element type
static_assert(std::is_same_v<decltype(lzw::encode_array<std::byte>([] { return tobe; })), std::array<std::byte, 18>>);
static_assert(std::is_same_v<decltype(lzw::encode_array<msb, std::byte>([] { return tobe; })), std::array<std::byte, 18>>);
constexpr auto tobe_chars = lzw::decode_array<char>([] { return tobe_lsb; });
static_assert(std::string_view(tobe_chars.data(), tobe_chars.size()) == tobe);
static_assert(std::ranges::equal(lzw::decode_array<msb, char>([] { return tobe_msb; }), tobe));

static_assert(lzw::encode_array([] { return ""sv; }).empty());
static_assert(lzw::encode_array([] { return std::vector<unsigned char>(tobe.begin(), tobe.end()); }) == tobe_lsb);
static_assert(lzw::decode_array([] { return std::array<unsigned char, 0>{}; }).empty());

// the plain functions are usable in constant evaluation too
static_assert([] {
  auto const encoded = lzw::encode<msb>(tobe);
  auto const decoded = lzw::decode<msb, char>(encoded);
  return std::ranges::equal(encoded, tobe_msb) && std::ranges::equal(decoded, tobe);
}());

// 1000 codes: enough to grow the code width and freeze the dictionary while
// constant-evaluated, and few enough for Clang's default -fconstexpr-steps
static_assert([] {
  constexpr lzw::options small{.max_width = 10};
  std::array<unsigned char, 1000> input{};
  for (std::size_t i = 0; i < input.size(); ++i) {
    input[i] = static_cast<unsigned char>((i * i + i / 7) % 256);
  }
  return std::ranges::equal(lzw::decode<small>(lzw::encode<small>(input)), input);
}());

}  // namespace

TEST_CASE("compile-time results are usable at runtime", "[compile_time]")
{
  CHECK(tobe_encoded == tobe_lsb);
  CHECK(std::ranges::equal(tobe_decoded, tobe, {}, [](char c) { return static_cast<unsigned char>(c); }));
}
