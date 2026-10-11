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
#pragma once
#include <algorithm>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include "core/integers.h"
#include "core/langs/expr/options.hpp"
#include "core/langs/expr/value.hpp"
#include "core/math/bitmanip/enums.hpp"

// Syntax of expressions shared by the assemblers and the debugger.
namespace pepp::tc::expr {

// Index of a node within its Tree.
using NodeId = u32;

enum class UnaryOp : u8 {
  Plus,
  Minus,
  BitNot,
  LogicalNot,
};

enum class BinaryOp : u8 {
  Multiply,
  Divide,
  Modulo,
  Add,
  Subtract,
  ShiftLeft,
  ShiftRight,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  Equal,
  NotEqual,
  BitAnd,
  BitXor,
  BitOr,
  LogicalAnd,
  LogicalOr,
};

// Bit masks (with one kind per leaf node, parentheses, and operation). Used to quickly compare the structure of trees, especially
// in the assembler while generating relocations. May need to widen to u64 if more operators are added.
enum class Kind : u32 {
  None = 0,
  Integer = 1u,
  Character = Integer << 1,
  Identifier = Character << 1,
  LocationCounter = Identifier << 1,
  FoldedConstant = LocationCounter << 1,
  Parens = FoldedConstant << 1,
  // A call to a function.
  ConstExprCall = Parens << 1,
  NonConstExprCall = ConstExprCall << 1,
  // UnaryOp in declaration order.
  UnaryStart = NonConstExprCall << 1,
  Plus = UnaryStart,
  Minus = Plus << 1,
  BitNot = Minus << 1,
  LogicalNot = BitNot << 1,
  UnaryLast = LogicalNot,
  // BinaryOp in declaration order.
  BinaryStart = UnaryLast << 1,
  Multiply = BinaryStart,
  Divide = Multiply << 1,
  Modulo = Divide << 1,
  Add = Modulo << 1,
  Subtract = Add << 1,
  ShiftLeft = Subtract << 1,
  ShiftRight = ShiftLeft << 1,
  Less = ShiftRight << 1,
  LessEqual = Less << 1,
  Greater = LessEqual << 1,
  GreaterEqual = Greater << 1,
  Equal = GreaterEqual << 1,
  NotEqual = Equal << 1,
  BitAnd = NotEqual << 1,
  BitXor = BitAnd << 1,
  BitOr = BitXor << 1,
  LogicalAnd = BitOr << 1,
  LogicalOr = LogicalAnd << 1,
  BinaryLast = LogicalOr,
  End = BinaryLast,
  // Convenient aliases for common types. A range [Start, Last] of single bits is (Last << 1) - Start.
  Constant = Integer | Character | FoldedConstant,
  // Values which are not known until the program is laid out.
  Symbolic = Identifier | LocationCounter,
  AnyCall = ConstExprCall | NonConstExprCall,
  AnyUnary = (UnaryLast << 1) - UnaryStart,
  AnyBinary = (BinaryLast << 1) - BinaryStart,
  Any = (End << 1) - 1,
};
consteval void is_bitflags(Kind);

constexpr Kind kind(UnaryOp op) {
  return static_cast<Kind>(bits::to_underlying(Kind::UnaryStart) << static_cast<u32>(op));
}
constexpr Kind kind(BinaryOp op) {
  return static_cast<Kind>(bits::to_underlying(Kind::BinaryStart) << static_cast<u32>(op));
}
static_assert(kind(UnaryOp::Plus) == Kind::Plus && kind(UnaryOp::LogicalNot) == Kind::LogicalNot,
              "Kind must mirror UnaryOp");
static_assert(kind(BinaryOp::Multiply) == Kind::Multiply && kind(BinaryOp::LogicalOr) == Kind::LogicalOr,
              "Kind must mirror BinaryOp");
static_assert(bits::to_underlying(Kind::Any) ==
                  (bits::to_underlying(Kind::Integer) | bits::to_underlying(Kind::Character) |
                   bits::to_underlying(Kind::Identifier) | bits::to_underlying(Kind::LocationCounter) |
                   bits::to_underlying(Kind::FoldedConstant) |
                   bits::to_underlying(Kind::Parens) | bits::to_underlying(Kind::AnyCall) |
                   bits::to_underlying(Kind::AnyUnary) | bits::to_underlying(Kind::AnyBinary)),
              "Kind's masks must cover every kind exactly");

struct Integer {
  static constexpr Kind KIND = Kind::Integer;
  enum class Format : u8 { Decimal, Hexadecimal };
  u64 value = 0;
  Format format = Format::Decimal;
};

// A character constant is treated as an integer, but retains the original text to  preserve escape sequences vs
// literals.
struct Character {
  static constexpr Kind KIND = Kind::Character;
  u8 value = 0;
  std::string text;
};

// Does not store identifier via lexer's string pool so that the tree may outlive parser
struct Identifier {
  static constexpr Kind KIND = Kind::Identifier;
  std::string name;
};

// The address of the instruction / directive beting emitted. All . in the same expression refer to the same symbol. The
// name of the symbol is unique for each address.
struct LocationCounter {
  static constexpr Kind KIND = Kind::LocationCounter;
  std::string name;
};

// A value computed during constant folding. Unlike other constants, it must record its type because it may have been
// widened or narrowed during compuation.
struct FoldedConstant {
  static constexpr Kind KIND = Kind::FoldedConstant;
  Value value;
};

// While the tree already encodes grouping, parentheses nodes are used to preserve the formatting of the input.
struct Parens {
  static constexpr Kind KIND = Kind::Parens;
  NodeId inner;
};

// A call to a function defined in Options, e.g. %hi(sym).
struct Call {
  static constexpr Kind KIND = Kind::AnyCall;
  const Function *function;
  NodeId argument;
};

struct Unary {
  static constexpr Kind KIND = Kind::AnyUnary;
  UnaryOp op;
  NodeId operand;
};

struct Binary {
  static constexpr Kind KIND = Kind::AnyBinary;
  BinaryOp op;
  NodeId lhs, rhs;
};

using Node = std::variant<Integer, Character, Identifier, LocationCounter, FoldedConstant, Parens, Call, Unary,
                          Binary>;
Kind kind(const Node &node);

// A syntax tree of nodes stored in a postordered, flat vector. Nodes refer to each other by index. By definition,
// children precede parents implying that the root is the most recently inserted item.
class Tree {
public:
  // Append a node whose operands are already in the tree. This node becomes the new root.
  NodeId add(Node node);

  bool empty() const { return _nodes.empty(); }
  NodeId root() const { return static_cast<NodeId>(_nodes.size() - 1); }
  const Node &operator[](NodeId id) const { return _nodes[id]; }
  const std::vector<Node> &nodes() const { return _nodes; }
  // Each position has 1 bit set indicating the concrete type of each node.
  const std::vector<Kind> &kinds() const { return _kinds; }

private:
  // SoA layout for cache efficiency.
  std::vector<Node> _nodes;
  std::vector<Kind> _kinds;
};

std::string_view to_string(UnaryOp op);
std::string_view to_string(BinaryOp op);
// std::nullopt if text is not a valid operator
std::optional<UnaryOp> unary_op(std::string_view text);
std::optional<BinaryOp> binary_op(std::string_view text);

// Return the relative precendence of the operator, with higher numbers binding more tightly. Matches C precendence, and
// assumes left-associativity.
int precedence(BinaryOp op);

// Serialize the tree in postfix notation, e.g. 1 + 2 * -x is "1 2 x u- * +". Unary plus and minus are written u+ and u-
// to tell them from their binary forms. Parens are omitted.
std::string to_postfix(const Tree &tree);
// Serialize the tree in infix notation, keeping Parens and otherwise inserting parentheses only as required for
// correctness.
std::string to_infix(const Tree &tree);

// A copy of the tree without explicit Parens nodes.
Tree strip_parens(const Tree &tree);

// Tree whose leaf nodes are constants and interior nodes that are constexpr operators or functions.
bool is_constant_expression(const Tree &tree);
// True if the tree uses the location counter (`.`).
bool uses_location_counter(const Tree &tree);

// Returns true if the two sequences &'ed together are non-zero for each position.
constexpr bool matches(std::span<const Kind> kinds, std::span<const Kind> pattern) {
  using namespace bits;
  // Compares sizes first, then stops at the first position whose AND is zero.
  return std::ranges::equal(kinds, pattern, [](Kind k, Kind p) { return any(k & p); });
}

} // namespace pepp::tc::expr
