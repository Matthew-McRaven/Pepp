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
#include "core/langs/expr/traversal.hpp"
#include <catch.hpp>
#include <string>
#include <vector>
#include "core/langs/expr/parser.hpp"

TEST_CASE("Expression tree traversal", "[scope:core][scope:core.langs][kind:unit][arch:*]") {
  using namespace pepp::tc::expr;
  const auto tree = [](const char *source) {
    auto result = parse(source);
    REQUIRE(std::holds_alternative<Parsed>(result));
    return std::get<Parsed>(std::move(result)).tree;
  };
  SECTION("Visit nodes of a single type") {
    std::vector<std::string> names;
    for_each_node<Identifier>(tree("a * (b - a) + 1"), [&](const Identifier &id) { names.push_back(id.name); });
    CHECK(names == std::vector<std::string>{"a", "b", "a"});
  }
  SECTION("Walk from the root") {
    const auto t = tree("a * (b - c) + d");
    const auto order = [&](auto &&descend) {
      std::string ret;
      walk(t, [&](NodeId id, const Node &node) {
        if (const auto *identifier = std::get_if<Identifier>(&node)) ret += identifier->name;
        else if (const auto *binary = std::get_if<Binary>(&node)) ret += to_string(binary->op);
        else if (std::holds_alternative<Parens>(node)) ret += "()";
        return descend(id, node);
      });
      return ret;
    };
    // Each node before its operands, left to right.
    CHECK(order([](NodeId, const Node &) { return true; }) == "+*a()-bcd");
    // A node which is not descended into hides its operands.
    CHECK(order([](NodeId, const Node &node) { return !std::holds_alternative<Parens>(node); }) == "+*a()d");
  }
  SECTION("Flatten expression chains") {
    // Stops at any node outside the group, and the right of each inverse flips inversion.
    const auto operands = [&](const char *source, const Group &group) {
      const auto t = strip_parens(tree(source));
      std::vector<std::string> ret;
      for (const auto &[id, inverted] : flatten(t, t.root(), group)) {
        const auto *identifier = std::get_if<Identifier>(&t[id]);
        ret.push_back((inverted ? "~" : "") + (identifier ? identifier->name : "?"));
      }
      return ret;
    };
    const Group add{BinaryOp::Add, BinaryOp::Subtract, 0}, multiply{BinaryOp::Multiply, std::nullopt, 1};
    CHECK(operands("a - (b - c) + d * e", add) == std::vector<std::string>{"a", "~b", "c", "?"});
    CHECK(operands("a * (b * c) * (d + e)", multiply) == std::vector<std::string>{"a", "b", "c", "?"});
    // A node outside the group is a single operand.
    CHECK(operands("a * b", add) == std::vector<std::string>{"?"});
    // Parens are outside every group, so they end a chain unless stripped.
    const auto t = tree("a - (b - c)");
    CHECK(flatten(t, t.root(), add).size() == 2);
  }
}
