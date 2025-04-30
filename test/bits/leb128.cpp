//===- llvm/unittest/Support/LEB128Test.cpp - LEB128 function tests -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "utils/bits/leb128.hpp"
#include <catch.hpp>
#include <string>
using namespace bits;

TEST_CASE("LEB128Test -- EncodeSLEB128", "[scope:bits][kind:unit][arch:*]") {
  // clang-format off
#define EXPECT_SLEB128_EQ(EXPECTED, VALUE, PAD) \
  do { \
        std::string Expected(EXPECTED, sizeof(EXPECTED) - 1); \
        uint8_t Buffer[32]; \
        unsigned Size = encodeSLEB128(VALUE, Buffer, PAD); \
        std::string Actual2(reinterpret_cast<const char *>(Buffer), Size); \
        CHECK(Expected == Actual2); \
  } while (0)
  //clang-format on
  
  // Encode SLEB128
  EXPECT_SLEB128_EQ("\x00", 0, 0);
  EXPECT_SLEB128_EQ("\x01", 1, 0);
  EXPECT_SLEB128_EQ("\x7f", -1, 0);
  EXPECT_SLEB128_EQ("\x3f", 63, 0);
  EXPECT_SLEB128_EQ("\x41", -63, 0);
  EXPECT_SLEB128_EQ("\x40", -64, 0);
  EXPECT_SLEB128_EQ("\xbf\x7f", -65, 0);
  EXPECT_SLEB128_EQ("\xc0\x00", 64, 0);

  // Encode SLEB128 with some extra padding bytes
  EXPECT_SLEB128_EQ("\x80\x00", 0, 2);
  EXPECT_SLEB128_EQ("\x80\x80\x00", 0, 3);
  EXPECT_SLEB128_EQ("\xff\x80\x00", 0x7f, 3);
  EXPECT_SLEB128_EQ("\xff\x80\x80\x00", 0x7f, 4);
  EXPECT_SLEB128_EQ("\x80\x81\x00", 0x80, 3);
  EXPECT_SLEB128_EQ("\x80\x81\x80\x00", 0x80, 4);
  EXPECT_SLEB128_EQ("\xc0\x7f", -0x40, 2);

  EXPECT_SLEB128_EQ("\xc0\xff\x7f", -0x40, 3);
  EXPECT_SLEB128_EQ("\x80\xff\x7f", -0x80, 3);
  EXPECT_SLEB128_EQ("\x80\xff\xff\x7f", -0x80, 4);
}

TEST_CASE("LEB128Test -- EncodeULEB128", "[scope:bits][kind:unit][arch:*]") {
  // clang-format off
#define EXPECT_ULEB128_EQ(EXPECTED, VALUE, PAD) \
  do { \
        std::string Expected(EXPECTED, sizeof(EXPECTED) - 1); \
        uint8_t Buffer[32]; \
        unsigned Size = encodeULEB128(VALUE, Buffer, PAD); \
        std::string Actual2(reinterpret_cast<const char *>(Buffer), Size); \
        CHECK(Expected == Actual2); \
  } while (0)
  //clang-format on
  // Encode ULEB128
  EXPECT_ULEB128_EQ("\x00", 0, 0);
  EXPECT_ULEB128_EQ("\x01", 1, 0);
  EXPECT_ULEB128_EQ("\x3f", 63, 0);
  EXPECT_ULEB128_EQ("\x40", 64, 0);
  EXPECT_ULEB128_EQ("\x7f", 0x7f, 0);
  EXPECT_ULEB128_EQ("\x80\x01", 0x80, 0);
  EXPECT_ULEB128_EQ("\x81\x01", 0x81, 0);
  EXPECT_ULEB128_EQ("\x90\x01", 0x90, 0);
  EXPECT_ULEB128_EQ("\xff\x01", 0xff, 0);
  EXPECT_ULEB128_EQ("\x80\x02", 0x100, 0);
  EXPECT_ULEB128_EQ("\x81\x02", 0x101, 0);

  // Encode ULEB128 with some extra padding bytes
  EXPECT_ULEB128_EQ("\x80\x00", 0, 2);
  EXPECT_ULEB128_EQ("\x80\x80\x00", 0, 3);
  EXPECT_ULEB128_EQ("\xff\x00", 0x7f, 2);
  EXPECT_ULEB128_EQ("\xff\x80\x00", 0x7f, 3);
  EXPECT_ULEB128_EQ("\x80\x81\x00", 0x80, 3);
  EXPECT_ULEB128_EQ("\x80\x81\x80\x00", 0x80, 4);
}

TEST_CASE("LEB128Test -- DecodeULEB128", "[scope:bits][kind:unit][arch:*]") {
  //clang-format off
#define EXPECT_DECODE_ULEB128_EQ(EXPECTED, VALUE) \
  do { \
    unsigned ActualSize = 0; \
    uint64_t Actual = decodeULEB128(reinterpret_cast<const uint8_t *>(VALUE), \
                                    &ActualSize); \
    CHECK(sizeof(VALUE) - 1 == ActualSize); \
    CHECK(EXPECTED == Actual); \
  } while (0)
  //clang-format on
  // Decode ULEB128
  EXPECT_DECODE_ULEB128_EQ(0u, "\x00");
  EXPECT_DECODE_ULEB128_EQ(1u, "\x01");
  EXPECT_DECODE_ULEB128_EQ(63u, "\x3f");
  EXPECT_DECODE_ULEB128_EQ(64u, "\x40");
  EXPECT_DECODE_ULEB128_EQ(0x7fu, "\x7f");
  EXPECT_DECODE_ULEB128_EQ(0x80u, "\x80\x01");
  EXPECT_DECODE_ULEB128_EQ(0x81u, "\x81\x01");
  EXPECT_DECODE_ULEB128_EQ(0x90u, "\x90\x01");
  EXPECT_DECODE_ULEB128_EQ(0xffu, "\xff\x01");
  EXPECT_DECODE_ULEB128_EQ(0x100u, "\x80\x02");
  EXPECT_DECODE_ULEB128_EQ(0x101u, "\x81\x02");
  EXPECT_DECODE_ULEB128_EQ(4294975616ULL, "\x80\xc1\x80\x80\x10");

  // Decode ULEB128 with extra padding bytes
  EXPECT_DECODE_ULEB128_EQ(0u, "\x80\x00");
  EXPECT_DECODE_ULEB128_EQ(0u, "\x80\x80\x00");
  EXPECT_DECODE_ULEB128_EQ(0x7fu, "\xff\x00");
  EXPECT_DECODE_ULEB128_EQ(0x7fu, "\xff\x80\x00");
  EXPECT_DECODE_ULEB128_EQ(0x80u, "\x80\x81\x00");
  EXPECT_DECODE_ULEB128_EQ(0x80u, "\x80\x81\x80\x00");
}

TEST_CASE("LEB128Test -- DecodeSLEB128", "[scope:bits][kind:unit][arch:*]") {
  //clang-format off
#define EXPECT_DECODE_SLEB128_EQ(EXPECTED, VALUE) \
  do { \
    unsigned ActualSize = 0; \
    uint64_t Actual = decodeSLEB128(reinterpret_cast<const uint8_t *>(VALUE), \
                                    &ActualSize); \
    CHECK(sizeof(VALUE) - 1 == ActualSize); \
    CHECK(EXPECTED == Actual); \
  } while (0)
  //clang-format on

  // Decode SLEB128
  EXPECT_DECODE_SLEB128_EQ(0L, "\x00");
  EXPECT_DECODE_SLEB128_EQ(1L, "\x01");
  EXPECT_DECODE_SLEB128_EQ(63L, "\x3f");
  EXPECT_DECODE_SLEB128_EQ(-64L, "\x40");
  EXPECT_DECODE_SLEB128_EQ(-63L, "\x41");
  EXPECT_DECODE_SLEB128_EQ(-1L, "\x7f");
  EXPECT_DECODE_SLEB128_EQ(128L, "\x80\x01");
  EXPECT_DECODE_SLEB128_EQ(129L, "\x81\x01");
  EXPECT_DECODE_SLEB128_EQ(-129L, "\xff\x7e");
  EXPECT_DECODE_SLEB128_EQ(-128L, "\x80\x7f");
  EXPECT_DECODE_SLEB128_EQ(-127L, "\x81\x7f");
  EXPECT_DECODE_SLEB128_EQ(64L, "\xc0\x00");
  EXPECT_DECODE_SLEB128_EQ(-12345L, "\xc7\x9f\x7f");

  // Decode unnormalized SLEB128 with extra padding bytes.
  EXPECT_DECODE_SLEB128_EQ(0L, "\x80\x00");
  EXPECT_DECODE_SLEB128_EQ(0L, "\x80\x80\x00");
  EXPECT_DECODE_SLEB128_EQ(0x7fL, "\xff\x00");
  EXPECT_DECODE_SLEB128_EQ(0x7fL, "\xff\x80\x00");
  EXPECT_DECODE_SLEB128_EQ(0x80L, "\x80\x81\x00");
  EXPECT_DECODE_SLEB128_EQ(0x80L, "\x80\x81\x80\x00");
}

TEST_CASE("LEB128Test -- SLEB128Size", "[scope:bits][kind:unit][arch:*]") {
  // Positive Value Testing Plan:
  // (1) 128 ^ n - 1 ........ need (n+1) bytes
  // (2) 128 ^ n ............ need (n+1) bytes
  // (3) 128 ^ n * 63 ....... need (n+1) bytes
  // (4) 128 ^ n * 64 - 1 ... need (n+1) bytes
  // (5) 128 ^ n * 64 ....... need (n+2) bytes

  CHECK(1u == getSLEB128Size(0x0LL));
  CHECK(1u == getSLEB128Size(0x1LL));
  CHECK(1u == getSLEB128Size(0x3fLL));
  CHECK(1u == getSLEB128Size(0x3fLL));
  CHECK(2u == getSLEB128Size(0x40LL));

  CHECK(2u == getSLEB128Size(0x7fLL));
  CHECK(2u == getSLEB128Size(0x80LL));
  CHECK(2u == getSLEB128Size(0x1f80LL));
  CHECK(2u == getSLEB128Size(0x1fffLL));
  CHECK(3u == getSLEB128Size(0x2000LL));

  CHECK(3u == getSLEB128Size(0x3fffLL));
  CHECK(3u == getSLEB128Size(0x4000LL));
  CHECK(3u == getSLEB128Size(0xfc000LL));
  CHECK(3u == getSLEB128Size(0xfffffLL));
  CHECK(4u == getSLEB128Size(0x100000LL));

  CHECK(4u == getSLEB128Size(0x1fffffLL));
  CHECK(4u == getSLEB128Size(0x200000LL));
  CHECK(4u == getSLEB128Size(0x7e00000LL));
  CHECK(4u == getSLEB128Size(0x7ffffffLL));
  CHECK(5u == getSLEB128Size(0x8000000LL));

  CHECK(5u == getSLEB128Size(0xfffffffLL));
  CHECK(5u == getSLEB128Size(0x10000000LL));
  CHECK(5u == getSLEB128Size(0x3f0000000LL));
  CHECK(5u == getSLEB128Size(0x3ffffffffLL));
  CHECK(6u == getSLEB128Size(0x400000000LL));

  CHECK(6u == getSLEB128Size(0x7ffffffffLL));
  CHECK(6u == getSLEB128Size(0x800000000LL));
  CHECK(6u == getSLEB128Size(0x1f800000000LL));
  CHECK(6u == getSLEB128Size(0x1ffffffffffLL));
  CHECK(7u == getSLEB128Size(0x20000000000LL));

  CHECK(7u == getSLEB128Size(0x3ffffffffffLL));
  CHECK(7u == getSLEB128Size(0x40000000000LL));
  CHECK(7u == getSLEB128Size(0xfc0000000000LL));
  CHECK(7u == getSLEB128Size(0xffffffffffffLL));
  CHECK(8u == getSLEB128Size(0x1000000000000LL));

  CHECK(8u == getSLEB128Size(0x1ffffffffffffLL));
  CHECK(8u == getSLEB128Size(0x2000000000000LL));
  CHECK(8u == getSLEB128Size(0x7e000000000000LL));
  CHECK(8u == getSLEB128Size(0x7fffffffffffffLL));
  CHECK(9u == getSLEB128Size(0x80000000000000LL));

  CHECK(9u == getSLEB128Size(0xffffffffffffffLL));
  CHECK(9u == getSLEB128Size(0x100000000000000LL));
  CHECK(9u == getSLEB128Size(0x3f00000000000000LL));
  CHECK(9u == getSLEB128Size(0x3fffffffffffffffLL));
  CHECK(10u == getSLEB128Size(0x4000000000000000LL));

  CHECK(10u == getSLEB128Size(0x7fffffffffffffffLL));
  CHECK(10u == getSLEB128Size(INT64_MAX));

  // Negative Value Testing Plan:
  // (1) - 128 ^ n - 1 ........ need (n+1) bytes
  // (2) - 128 ^ n ............ need (n+1) bytes
  // (3) - 128 ^ n * 63 ....... need (n+1) bytes
  // (4) - 128 ^ n * 64 ....... need (n+1) bytes (different from positive one)
  // (5) - 128 ^ n * 65 - 1 ... need (n+2) bytes (if n > 0)
  // (6) - 128 ^ n * 65 ....... need (n+2) bytes

  CHECK(1u == getSLEB128Size(0x0LL));
  CHECK(1u == getSLEB128Size(-0x1LL));
  CHECK(1u == getSLEB128Size(-0x3fLL));
  CHECK(1u == getSLEB128Size(-0x40LL));
  CHECK(1u == getSLEB128Size(-0x40LL)); // special case
  CHECK(2u == getSLEB128Size(-0x41LL));

  CHECK(2u == getSLEB128Size(-0x7fLL));
  CHECK(2u == getSLEB128Size(-0x80LL));
  CHECK(2u == getSLEB128Size(-0x1f80LL));
  CHECK(2u == getSLEB128Size(-0x2000LL));
  CHECK(3u == getSLEB128Size(-0x207fLL));
  CHECK(3u == getSLEB128Size(-0x2080LL));

  CHECK(3u == getSLEB128Size(-0x3fffLL));
  CHECK(3u == getSLEB128Size(-0x4000LL));
  CHECK(3u == getSLEB128Size(-0xfc000LL));
  CHECK(3u == getSLEB128Size(-0x100000LL));
  CHECK(4u == getSLEB128Size(-0x103fffLL));
  CHECK(4u == getSLEB128Size(-0x104000LL));

  CHECK(4u == getSLEB128Size(-0x1fffffLL));
  CHECK(4u == getSLEB128Size(-0x200000LL));
  CHECK(4u == getSLEB128Size(-0x7e00000LL));
  CHECK(4u == getSLEB128Size(-0x8000000LL));
  CHECK(5u == getSLEB128Size(-0x81fffffLL));
  CHECK(5u == getSLEB128Size(-0x8200000LL));

  CHECK(5u == getSLEB128Size(-0xfffffffLL));
  CHECK(5u == getSLEB128Size(-0x10000000LL));
  CHECK(5u == getSLEB128Size(-0x3f0000000LL));
  CHECK(5u == getSLEB128Size(-0x400000000LL));
  CHECK(6u == getSLEB128Size(-0x40fffffffLL));
  CHECK(6u == getSLEB128Size(-0x410000000LL));

  CHECK(6u == getSLEB128Size(-0x7ffffffffLL));
  CHECK(6u == getSLEB128Size(-0x800000000LL));
  CHECK(6u == getSLEB128Size(-0x1f800000000LL));
  CHECK(6u == getSLEB128Size(-0x20000000000LL));
  CHECK(7u == getSLEB128Size(-0x207ffffffffLL));
  CHECK(7u == getSLEB128Size(-0x20800000000LL));

  CHECK(7u == getSLEB128Size(-0x3ffffffffffLL));
  CHECK(7u == getSLEB128Size(-0x40000000000LL));
  CHECK(7u == getSLEB128Size(-0xfc0000000000LL));
  CHECK(7u == getSLEB128Size(-0x1000000000000LL));
  CHECK(8u == getSLEB128Size(-0x103ffffffffffLL));
  CHECK(8u == getSLEB128Size(-0x1040000000000LL));

  CHECK(8u == getSLEB128Size(-0x1ffffffffffffLL));
  CHECK(8u == getSLEB128Size(-0x2000000000000LL));
  CHECK(8u == getSLEB128Size(-0x7e000000000000LL));
  CHECK(8u == getSLEB128Size(-0x80000000000000LL));
  CHECK(9u == getSLEB128Size(-0x81ffffffffffffLL));
  CHECK(9u == getSLEB128Size(-0x82000000000000LL));

  CHECK(9u == getSLEB128Size(-0xffffffffffffffLL));
  CHECK(9u == getSLEB128Size(-0x100000000000000LL));
  CHECK(9u == getSLEB128Size(-0x3f00000000000000LL));
  CHECK(9u == getSLEB128Size(-0x4000000000000000LL));
  CHECK(10u == getSLEB128Size(-0x40ffffffffffffffLL));
  CHECK(10u == getSLEB128Size(-0x4100000000000000LL));

  CHECK(10u == getSLEB128Size(-0x7fffffffffffffffLL));
  CHECK(10u == getSLEB128Size(-0x8000000000000000LL));
  CHECK(10u == getSLEB128Size(INT64_MIN));
}

TEST_CASE("LEB128Test -- ULEB128Size", "[scope:bits][kind:unit][arch:*]") {
  // Testing Plan:
  // (1) 128 ^ n ............ need (n+1) bytes
  // (2) 128 ^ n * 64 ....... need (n+1) bytes
  // (3) 128 ^ (n+1) - 1 .... need (n+1) bytes

  CHECK(1u == getULEB128Size(0)); // special case

  CHECK(1u == getULEB128Size(0x1ULL));
  CHECK(1u == getULEB128Size(0x40ULL));
  CHECK(1u == getULEB128Size(0x7fULL));

  CHECK(2u == getULEB128Size(0x80ULL));
  CHECK(2u == getULEB128Size(0x2000ULL));
  CHECK(2u == getULEB128Size(0x3fffULL));

  CHECK(3u == getULEB128Size(0x4000ULL));
  CHECK(3u == getULEB128Size(0x100000ULL));
  CHECK(3u == getULEB128Size(0x1fffffULL));

  CHECK(4u == getULEB128Size(0x200000ULL));
  CHECK(4u == getULEB128Size(0x8000000ULL));
  CHECK(4u == getULEB128Size(0xfffffffULL));

  CHECK(5u == getULEB128Size(0x10000000ULL));
  CHECK(5u == getULEB128Size(0x400000000ULL));
  CHECK(5u == getULEB128Size(0x7ffffffffULL));

  CHECK(6u == getULEB128Size(0x800000000ULL));
  CHECK(6u == getULEB128Size(0x20000000000ULL));
  CHECK(6u == getULEB128Size(0x3ffffffffffULL));

  CHECK(7u == getULEB128Size(0x40000000000ULL));
  CHECK(7u == getULEB128Size(0x1000000000000ULL));
  CHECK(7u == getULEB128Size(0x1ffffffffffffULL));

  CHECK(8u == getULEB128Size(0x2000000000000ULL));
  CHECK(8u == getULEB128Size(0x80000000000000ULL));
  CHECK(8u == getULEB128Size(0xffffffffffffffULL));

  CHECK(9u == getULEB128Size(0x100000000000000ULL));
  CHECK(9u == getULEB128Size(0x4000000000000000ULL));
  CHECK(9u == getULEB128Size(0x7fffffffffffffffULL));

  CHECK(10u == getULEB128Size(0x8000000000000000ULL));

  CHECK(10u == getULEB128Size(UINT64_MAX));
}
