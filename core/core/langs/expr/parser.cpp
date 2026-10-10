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
#include "core/langs/expr/parser.hpp"
#include <algorithm>
#include <optional>
#include "core/compile/lex/buffer.hpp"
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb/asmb_tokens.hpp"
#include "core/langs/expr/lexer.hpp"
#include "core/math/bitmanip/strings.hpp"

namespace {
using namespace pepp::tc::expr;
using pepp::tc::expr::Integer;
namespace lex = pepp::tc::lex;
using pepp::tc::support::Location;
using pepp::tc::support::LocationInterval;

// Thrown if the parser encounters a syntax error in the middle of an expression. e.g., `4 +` would throw this error.
struct Failure {
  LocationInterval location;
  std::string message;
};

/*
 * Implemented as a Pratt parser rather than recursive descent because precedence climbing is so easy to implement.
 * The old debug watch expression parser had multiple associativity errors from being implemented via recursive descent.
 * See: https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
 * TODO: will need right associativity to allow `struct.member` style access in debugger.
 */
class Parser {
public:
  Parser(lex::Buffer &buffer, Location origin, const Options &options, const NameLocationCounter &location_counter)
      : _buf(buffer), _options(options), _name_location_counter(location_counter), _origin(origin), _end(origin) {}

  // Returns nullopt if the next token cannot start an expression.
  std::optional<NodeId> expression(int min_precedence) {
    auto lhs = unary();
    if (!lhs) return std::nullopt;
    while (auto literal = _buf.peek<lex::Literal>()) {
      if (_options.dot == Options::Dot::Operator && literal->literal == ".")
        throw Failure{literal->location(), "Member access is not implemented"};
      const auto op = binary_op(literal->literal);
      if (!op || precedence(*op) < min_precedence) break;
      consume(_buf.match<lex::Literal>());
      // One level higher makes operators of equal precedence group to the left.
      const auto rhs = expression(precedence(*op) + 1);
      if (!rhs) expected_operand();
      lhs = add(Binary{*op, *lhs, *rhs}, LocationInterval(locations[*lhs].lower(), locations[*rhs].upper()));
    }
    return lhs;
  }

  // Number of characters consumed from the start of the text.
  size_t length() const { return static_cast<size_t>(_end.column - _origin.column); }

  Tree tree;
  std::vector<LocationInterval> locations;

private:
  NodeId add(Node node, LocationInterval location) {
    locations.emplace_back(location);
    return tree.add(std::move(node));
  }

  std::optional<NodeId> unary() {
    if (auto literal = _buf.peek<lex::Literal>(); literal) {
      if (const auto op = unary_op(literal->literal); op) {
        consume(_buf.match<lex::Literal>());
        const auto operand = unary();
        if (!operand) expected_operand();
        return add(Unary{*op, *operand}, LocationInterval(literal->location().lower(), locations[*operand].upper()));
      }
    }
    return primary();
  }

  std::optional<NodeId> primary() {
    if (auto integer = _buf.match<lex::Integer>(); integer) {
      consume(integer);
      const auto format = integer->format == lex::Integer::Format::Hex ? Integer::Format::Hexadecimal : Integer::Format::Decimal;
      return add(Integer{integer->value, format}, integer->location());
    } else if (auto character = _buf.match<lex::CharacterConstant>(); character) {
      consume(character);
      const auto value = bits::escapedToByte(character->value);
      if (!value) throw Failure{character->location(), "Invalid character constant"};
      return add(Character{*value, character->value}, character->location());
    } else if (auto identifier = _buf.match<lex::Identifier>(); identifier) {
      consume(identifier);
      const auto name = identifier->view();
      const auto function = std::ranges::find(_options.functions, name, &Function::name);
      if (function != _options.functions.end()) return call(*function, identifier->location());
      // Only functions may be named with a %.
      else if (name.starts_with('%')) throw Failure{identifier->location(), "Unknown function " + std::string(name)};
      return add(Identifier{std::string(name)}, identifier->location());
    } else if (_options.dot == Options::Dot::Identifier && _buf.peek_literal(".")) {
      const auto dot = _buf.match_literal(".");
      consume(dot);
      return add(LocationCounter{location_counter(dot->location())}, dot->location());
    } else if (_buf.peek_literal("(")) {
      const auto open = _buf.match_literal("(");
      consume(open);
      const auto inner = expression(0);
      if (!inner) expected_operand();
      if (!_buf.peek_literal(")")) throw Failure{_buf.peek()->location(), "Expected ')'"};
      const auto close = _buf.match_literal(")");
      consume(close);
      return add(Parens{*inner}, LocationInterval(open->location().lower(), close->location().upper()));
    }
    return std::nullopt;
  }

  // <function> ( <expression> )
  NodeId call(const Function &function, LocationInterval name) {
    if (!_buf.peek_literal("(")) throw Failure{_buf.peek()->location(), "Expected '('"};
    consume(_buf.match_literal("("));
    const auto argument = expression(0);
    if (!argument) expected_operand();
    if (!_buf.peek_literal(")")) throw Failure{_buf.peek()->location(), "Expected ')'"};
    const auto close = _buf.match_literal(")");
    consume(close);
    return add(Call{&function, *argument}, LocationInterval(name.lower(), close->location().upper()));
  }

  // All . in the same expression refer to the same symbol.
  const std::string &location_counter(LocationInterval location) {
    if (_location_counter) return *_location_counter;
    else if (!_name_location_counter) throw Failure{location, "The location counter is not available"};
    return *(_location_counter = _name_location_counter());
  }

  // Called after an operator or '(' when no operand follows it.
  [[noreturn]] void expected_operand() { throw Failure{_buf.peek()->location(), "Expected an operand"}; }

  void consume(const std::shared_ptr<lex::Token> &token) { _end = token->location().upper(); }

  lex::Buffer &_buf;
  const Options &_options;
  const NameLocationCounter &_name_location_counter;
  std::optional<std::string> _location_counter;
  Location _origin, _end;
};
} // namespace

pepp::tc::expr::ParseResult pepp::tc::expr::parse(support::SeekableData cursor, std::shared_ptr<IdentifierPool> pool,
                                                  const Options &options,
                                                  const NameLocationCounter &location_counter) {
  const auto origin = cursor.location();
  // The lexer reads one token past the end of the expression, but we need to return a cursor pointing immediately after
  // the last token of the expression.
  auto after = cursor;
  if (!pool) pool = std::make_shared<IdentifierPool>();
  ExpressionLexer lexer(std::move(pool), std::move(cursor), options);
  lex::Buffer buffer(&lexer);
  Parser parser(buffer, origin, options, location_counter);
  try {
    const auto root = parser.expression(0);
    if (!root) return NoExpression{};
    // Parser does not consume newlines meaning it only advances the column.
    after.skip(parser.length());
    return Parsed{std::move(parser.tree), std::move(parser.locations), parser.length(), std::move(after)};
  } catch (const Failure &failure) {
    return Error{failure.location, failure.message};
  }
}

pepp::tc::expr::ParseResult pepp::tc::expr::parse(std::string_view text, support::Location origin,
                                                  std::shared_ptr<IdentifierPool> pool, const Options &options,
                                                  const NameLocationCounter &location_counter) {
  return parse(support::SeekableData(std::string(text), origin), std::move(pool), options, location_counter);
}
