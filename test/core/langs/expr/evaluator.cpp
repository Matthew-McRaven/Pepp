/*
 * Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "core/langs/expr/evaluator.hpp"
#include <catch.hpp>
#include <limits>
#include <vector>
#include "core/langs/expr/parser.hpp"

namespace {
using namespace pepp::tc::expr;
using enum Signedness;

Tree tree_of(const char *source) {
  auto result = parse(source);
  REQUIRE(std::holds_alternative<Parsed>(result));
  return std::get<Parsed>(std::move(result)).tree;
}

std::expected<Value, Error> evaluate(const char *source, const Options &options) {
  return evaluate_constant(tree_of(source), options);
}
} // namespace

TEST_CASE("Expression evaluation", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  // Pep/10 has a 16-bit int and treats numbers as u16 by default.
  // RISC-V has a 32-bit int and is signed by default.
  const Options pep{16, Unsigned}, pep_signed{16, Signed}, rv{32, Signed};
  constexpr u64 max = std::numeric_limits<u64>::max();

  SECTION("Values and types") {
    struct Case {
      const char *source;
      Options options;
      Value value;
    };
    const std::vector<Case> cases = {
        // Literals are the smallest type which can hold the bit pattern (either signed or unsigned). Starts at
        // sizeof(int)
        {"42", pep, {42, {16, Bits}}},
        {"0xFFFF", pep, {0xFFFF, {16, Bits}}},
        {"65535", pep, {65535, {16, Bits}}},
        {"0x10000", pep, {0x10000, {32, Bits}}},
        {"0xFFFF", rv, {0xFFFF, {32, Bits}}},
        {"0xFFFFFFFFFFFFFFFF", pep, {max, {64, Bits}}},
        {"'a'", pep, {'a', {16, Bits}}},
        // Sign-agnostic operators preserve "bit" inputs
        {"0xFFFF + 1", pep, {0, {16, Bits}}},
        {"~0", pep, {0xFFFF, {16, Bits}}},
        {"1 << 15", pep, {0x8000, {16, Bits}}},
        // Negation forces signedness
        {"-1", pep, {0xFFFF, {16, Signed}}},
        {"-4 / 2", pep, {0xFFFE, {16, Signed}}},
        {"-16 >> 2", pep, {0xFFFC, {16, Signed}}},
        // Non-signed bits follow default
        {"0xFFFF / 2", pep, {0x7FFF, {16, Unsigned}}},
        {"0xFFFF / 2", pep_signed, {0, {16, Signed}}},
        {"0xFFFF >> 4", pep, {0x0FFF, {16, Unsigned}}},
        {"0xFFFF >> 4", pep_signed, {0xFFFF, {16, Signed}}},
        {"0x10000 + 0xFFFF", pep, {0x1FFFF, {32, Unsigned}}}, // Widening bits also needs a sign.
        // Logic and comparisons always result in an int that is 0 or 1.
        {"0xFFFF > -1", pep, {0, {16, Signed}}}, // As in C: 0xFFFF reads as -1 against a signed -1.
        {"0xFFFFFFFFFFFFFFFF > -1", pep, {0, {16, Signed}}},
        {"0xFFFF < 1", pep, {0, {16, Signed}}},
        {"0xFFFF < 1", pep_signed, {1, {16, Signed}}},
        {"3 == 3", pep, {1, {16, Signed}}},
        {"!5", pep, {0, {16, Signed}}},
        {"2 && 3", pep, {1, {16, Signed}}},
        {"0 && 1 / 0", pep, {0, {16, Signed}}}, // Short-circuits.
        {"1 || 1 / 0", pep, {1, {16, Signed}}},
    };
    for (const auto &c : cases) {
      CAPTURE(c.source, c.options.int_bits, static_cast<int>(c.options.default_sign));
      const auto result = evaluate(c.source, c.options);
      REQUIRE(result.has_value());
      CHECK(result->bits == c.value.bits);
      CHECK(result->type.bits == c.value.type.bits);
      CHECK(result->type.sign == c.value.type.sign);
    }
  }
  SECTION("Errors") {
    struct Case {
      const char *source, *message;
    };
    const std::vector<Case> cases = {
        {"1 / 0", "Division by zero"},
        {"1 % 0", "Division by zero"},
        {"1 && 1 / 0", "Division by zero"},
        {"(-32767 - 1) / -1", "Signed division overflow"},
        {"(-32767 - 1) % -1", "Signed division overflow"},
        {"1 << 16", "Shift amount out of range"},
        {"1 << -1", "Shift amount out of range"},
        {"sym + 1", "Symbols are not allowed in a constant expression"},
    };
    for (const auto &c : cases) {
      CAPTURE(c.source);
      const auto result = evaluate(c.source, pep);
      REQUIRE(!result.has_value());
      CHECK(result.error().message == c.message);
    }
    // Errors are located at the operation which failed.
    const auto result = evaluate("2 + 1 / 0", pep);
    REQUIRE(!result.has_value());
    CHECK(result.error().location.lower() == pepp::tc::support::Location(0, 4));
    CHECK(result.error().location.upper() == pepp::tc::support::Location(0, 9));
  }
}
