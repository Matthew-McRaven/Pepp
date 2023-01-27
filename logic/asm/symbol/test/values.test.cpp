#include "catch.hpp"

#include "symbol/types.hpp"
#include "symbol/value.hpp"

TEST_CASE("Test symbol values.") {

  // Check that bitmasks work.
  SECTION("Bitmask") {
    symbol::value::MaskedBits start{
        .byteCount = 1, .bitPattern = 0xf, .mask = 0x7};
    symbol::value::MaskedBits end{
        .byteCount = 1, .bitPattern = 0x7, .mask = 0xf};
    CHECK(start == start);
    CHECK(start != end);
    CHECK(start() == end());
  }
  SECTION("Empty Value") {
    auto value = symbol::value::Empty(0);
    CHECK_NOFAIL(value.value()());
    CHECK(value.value()() == 0);
  }

  SECTION("Deleted Value") {
    auto value = symbol::value::Deleted();
    CHECK_NOFAIL(value.value()());
    CHECK(value.value()() == 0);
    CHECK(value.type() == symbol::Type::kDeleted);
  }

  // Check that the values on a numeric value can be mutated.
  SECTION("Numeric Value") {
    symbol::value::MaskedBits start{
        .byteCount = 1,
        .bitPattern = 30,
        .mask = 0xff,
    };
    symbol::value::MaskedBits end{
        .byteCount = 1,
        .bitPattern = 20,
        .mask = 0xff,
    };
    auto value = symbol::value::Constant(start);
    CHECK_NOFAIL(value.value()());
    CHECK(value.value()() == start());
    CHECK_NOTHROW(value.setValue(end));
    CHECK(value.value()() == end());
  }

  // Check that the values on a location value can be mutated.
  SECTION("Location Value") {
    auto base = 7;
    auto start_offset = 11, end_offset = 13;
    auto value =
        symbol::value::Location(2, base, start_offset, symbol::Type::kCode);
    CHECK(value.value()() == base + start_offset);
    CHECK_NOTHROW(value.setOffset(end_offset));
    CHECK(value.value()() == base + end_offset);
    CHECK(value.relocatable());
  }
  // Can't test external symbol value here, as it will require a symbol table.
}
