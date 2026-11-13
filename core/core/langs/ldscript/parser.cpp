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
#include "core/langs/ldscript/parser.hpp"
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>
#include "core/compile/diagnostic/severity.hpp"
#include "core/compile/lex/buffer.hpp"
#include "core/compile/lex/tokens.hpp"
#include "core/langs/asmb/diagnostic_table.hpp"
#include "core/langs/expr/nested.hpp"
#include "core/langs/ldscript/expression.hpp"
#include "core/langs/ldscript/lexer.hpp"
#include "core/langs/ldscript/parser_error.hpp"

namespace {
using namespace pepp::tc::ldscript;
namespace expr = pepp::tc::expr;
namespace lex = pepp::tc::lex;
using pepp::tc::support::LocationInterval;
using TokenPtr = std::shared_ptr<lex::Token>;
using Identifier = std::shared_ptr<lex::Identifier>;
using NullaryError = ParserError::NullaryError;
using UnaryError = ParserError::UnaryError;

// Returns true if text is PROVIDE, PROVIDE_HIDDEN, or HIDDEN.
bool is_provide(std::string_view text) { return assignment_kind(text).has_value(); }
bool is_keep(std::string_view text) { return text == "KEEP"; }

bool is_comment(const TokenPtr &token) { return token->mask(lex::InlineComment::TYPE | lex::BlockComment::TYPE); }

// Returns the text of a literal token, or an empty string for any other token.
std::string_view literal_of(const TokenPtr &token) {
  if (!token->mask(lex::Literal::TYPE)) return {};
  return std::static_pointer_cast<lex::Literal>(token)->literal;
}

// Lexer that discards comments, since the IR cannot hold them yet.
// TODO: Keep comments in the IR so that formatted output does not drop them.
class Uncommented : public Lexer {
public:
  using Lexer::Lexer;
  std::shared_ptr<lex::Token> next_token() override {
    auto ret = Lexer::next_token();
    while (is_comment(ret)) ret = Lexer::next_token();
    return ret;
  }
  // Move the cursor past comments without lexing the token after them, so that an expression may start there.
  void skip_comments() {
    auto start = cursor();
    while (is_comment(Lexer::next_token())) start = cursor();
    resume_at(start);
  }
  // Lex the next token without consuming it and is only meaningful if token buffer is empty. Expression sub-parser
  // fails when there are buffered tokens. This function provides a way to lookahead without causing token buffering.
  TokenPtr lookahead() {
    const auto start = cursor();
    auto ret = next_token();
    resume_at(start);
    return ret;
  }
  // Returns true if the next token is an assignment operator, at which point we should enter the expression sub-parser.
  bool at_assignment() { return literal_of(lookahead()).ends_with('='); }
};

/*
 * Recursive descent parser for the subset of GNU ld's script language that we accept:
 *
 *   <script>         ::= (<command> | ;)*
 *   <command>        ::= <entry> | <input-files> | <output-format> | <output-arch> | <sections> | <provide>
 *                      | <assignment>
 *   <entry>          ::= ENTRY ( <name> )
 *   <input-files>    ::= (INPUT | GROUP) <name-list>
 *   <output-format>  ::= OUTPUT_FORMAT ( <name> [, <name> , <name>] )
 *   <output-arch>    ::= OUTPUT_ARCH ( <name> )
 *   <sections>       ::= SECTIONS { (<provide> | <assignment> | <output-section> | ;)* }
 *   <output-section> ::= <name> [<expression>] : { (<provide> | <keep> | <assignment> | <input-section> | ;)* }
 *   <input-section>  ::= <file> <name-list>
 *   <keep>           ::= KEEP ( <input-section> )
 *   <assignment>     ::= <name> (= | += | -= | *= | /= | <<= | >>= | &= | |=) <expression> ;
 *   <provide>        ::= (PROVIDE | PROVIDE_HIDDEN | HIDDEN) ( <name> = <expression> )
 *   <file>           ::= <name> [: [<name>]] | : <name>
 *   <name-list>      ::= ( (<name> [,])* )
 *
 * A <name> is an unquoted name or a quoted string, and an <expression> is parsed by the expr sub-parser. Comments are
 * discarded by the lexer.
 *
 * A rule that starts with a keyword is required to return a valid IR node or otherwise throw a parsing error. Other
 * rules return nullopt without consuming input if the input does not start with their construct.
 */
class Parser {
public:
  Parser(pepp::tc::DiagnosticTable &diag, pepp::tc::support::SeekableData text)
      : _diag(diag), _lexer(_pool, std::move(text)), _buf(&_lexer) {}

  // (<command> | ;)*
  Script script() {
    // Held for the whole parse so that the buffer never clears its tokens. Otherwise, a token peeked at the end of one
    // command would be lost, and Markers would be invalidated.
    const lex::Checkpoint keep_tokens(_buf);
    Script ret;
    while (!_buf.peek<lex::EoF>()) {
      const lex::Marker start(_buf);
      try {
        if (_buf.match_literal(";")) continue;
        else if (auto command = this->command()) ret.commands.emplace_back(std::move(*command));
        else throw invalid();
      } catch (const ParserError &e) {
        _diag.add_message(e.loc, e.what(), pepp::tc::Severity::Fatal);
        synchronize(start);
      }
    }
    return ret;
  }

private:
  // Adapt a rule to return a Script::Command, so that rules for different commands can share a table.
  template <auto rule> Script::Command as_command(const Identifier &keyword) { return (this->*rule)(keyword); }

  // <entry> | <input-files> | <output-format> | <output-arch> | <sections> | <provide> | <assignment>
  std::optional<Script::Command> command() {
    using Rule = Script::Command (Parser::*)(const Identifier &);
    // Rather than a if-else chain, dispatch directly to the handler for a each keyword.
    static const std::unordered_map<std::string_view, Rule> rules{
        {"ENTRY", &Parser::as_command<&Parser::entry>},
        {"INPUT", &Parser::as_command<&Parser::input_files>},
        {"GROUP", &Parser::as_command<&Parser::input_files>},
        {"OUTPUT_FORMAT", &Parser::as_command<&Parser::output_format>},
        {"OUTPUT_ARCH", &Parser::as_command<&Parser::output_arch>},
        {"SECTIONS", &Parser::as_command<&Parser::sections>},
        {"PROVIDE", &Parser::as_command<&Parser::provide>},
        {"PROVIDE_HIDDEN", &Parser::as_command<&Parser::provide>},
        {"HIDDEN", &Parser::as_command<&Parser::provide>},
    };
    const auto keyword = _buf.peek<lex::Identifier>();
    if (const auto rule = keyword ? rules.find(keyword->view()) : rules.end(); rule != rules.end())
      return (this->*rule->second)(_buf.match<lex::Identifier>());
    else if (auto assigned = assignment()) return std::move(*assigned);
    return std::nullopt;
  }

  // ENTRY ( <symbol> )
  Entry entry(const Identifier &keyword) {
    return Entry{std::string(parenthesized_name()->view()), matched_since(keyword)};
  }

  // INPUT|GROUP <name-list>
  InputFiles input_files(const Identifier &keyword) {
    const auto kind = keyword->view() == "GROUP" ? InputFiles::Kind::Group : InputFiles::Kind::Input;
    return InputFiles{kind, name_list(), matched_since(keyword)};
  }

  // OUTPUT_FORMAT ( <format> [, <big> , <little>] )
  OutputFormat output_format(const Identifier &keyword) {
    expect_literal("(");
    OutputFormat ret{format()};
    if (_buf.match_literal(",")) {
      ret.big = format();
      expect_literal(",");
      ret.little = format();
    }
    expect_literal(")");
    ret.source_interval = matched_since(keyword);
    return ret;
  }
  OutputFormat::Format format() {
    const auto token = name();
    if (!token) throw missing(NullaryError::Name_Expected);
    else if (const auto ret = pepp::tc::ldscript::output_format(token->view())) return *ret;
    throw ParserError(UnaryError::Format_Unknown, std::string(token->view()), token->location());
  }

  // OUTPUT_ARCH ( <architecture> )
  OutputArch output_arch(const Identifier &keyword) {
    const auto token = parenthesized_name();
    const auto machine = architecture(token->view());
    if (!machine) throw ParserError(UnaryError::Architecture_Unknown, std::string(token->view()), token->location());
    return OutputArch{*machine, matched_since(keyword)};
  }

  // SECTIONS { (<provide> | <assignment> | <output-section> | ;)* }
  Sections sections(const Identifier &keyword) {
    Sections ret;
    auto f = [&] {
      if (const auto provide_keyword = match_keyword(is_provide)) ret.items.emplace_back(provide(provide_keyword));
      else if (auto assigned = assignment()) ret.items.emplace_back(std::move(*assigned));
      else if (auto section = output_section()) ret.items.emplace_back(std::move(*section));
      else return false;
      return true;
    };
    braced(f);
    ret.source_interval = matched_since(keyword);
    return ret;
  }

  // <symbol> <op> <expression> ;
  std::optional<Assignment> assignment() {
    // Needed to roll back when a name is not followed by an assignment operator.
    lex::Checkpoint cp(_buf);
    const auto symbol = name();
    if (!symbol || !_lexer.at_assignment()) return cp.rollback(), std::nullopt;
    const auto op = _buf.match<lex::Literal>();
    auto value = expression();
    if (!value) throw missing(NullaryError::Expression_Expected);
    expect_literal(";");
    // The lexer only produces = or a binary operator followed by =.
    const auto &text = op->literal;
    const auto compound = text == "=" ? std::nullopt : expr::binary_op(text.substr(0, text.size() - 1));
    return Assignment{Assignment::Kind::Plain, std::string(symbol->view()), compound, std::move(*value),
                      matched_since(symbol)};
  }

  // PROVIDE|PROVIDE_HIDDEN|HIDDEN ( <symbol> = <expression> )
  // GNU ld allows a ; after the close paren for PROVIDE. Since the main parser loop skips stray ; tokens, we do not
  // need to consume it here.
  Assignment provide(const Identifier &keyword) {
    expect_literal("(");
    const auto symbol = name();
    if (!symbol) throw missing(NullaryError::Name_Expected);
    expect_literal("=");
    auto value = expression();
    if (!value) throw missing(NullaryError::Expression_Expected);
    expect_literal(")");
    const auto kind = *assignment_kind(keyword->view());
    return Assignment{kind, std::string(symbol->view()), std::nullopt, std::move(*value), matched_since(keyword)};
  }

  // <name> [<address>] : { (<provide> | <keep> | <assignment> | <input-section> | ;)* }
  std::optional<OutputSectionDescription> output_section() {
    const auto section_name = name();
    if (!section_name) return std::nullopt;
    const auto maybe_address = expression();
    OutputSectionDescription ret{.name = std::string(section_name->view()), .address = maybe_address};
    expect_literal(":");
    auto f = [&] {
      if (const auto provide_keyword = match_keyword(is_provide)) ret.items.emplace_back(provide(provide_keyword));
      else if (const auto keep_keyword = match_keyword(is_keep)) ret.items.emplace_back(keep(keep_keyword));
      else if (auto assigned = assignment()) ret.items.emplace_back(std::move(*assigned));
      else if (auto input = input_section()) ret.items.emplace_back(std::move(*input));
      else return false;
      return true;
    };
    braced(f);
    ret.source_interval = matched_since(section_name);
    return ret;
  }

  // KEEP ( <file> <name-list> )
  InputSectionDescription keep(const Identifier &keyword) {
    expect_literal("(");
    auto file = file_pattern();
    if (!file) throw missing(NullaryError::Name_Expected);
    auto sections = name_list();
    expect_literal(")");
    return InputSectionDescription{std::move(file->second), std::move(sections), true, matched_since(keyword)};
  }

  // <file> <name-list>
  std::optional<InputSectionDescription> input_section() {
    auto file = file_pattern();
    if (!file) return std::nullopt;
    return InputSectionDescription{std::move(file->second), name_list(), false, matched_since(file->first)};
  }

  // <name> [: [<name>]] | : <name>
  // Parse a file pattern, returning it with its first token. Returns nullopt if the following tokens are not a <name>.
  std::optional<std::pair<TokenPtr, FilePattern>> file_pattern() {
    using enum FilePattern::Match;
    using Ret = std::pair<TokenPtr, FilePattern>;
    if (const auto colon = _buf.match_literal(":")) {
      const auto file = name();
      if (!file) throw missing(NullaryError::Name_Expected);
      return Ret{colon, {NotInArchive, std::string(file->view())}};
    }
    const auto first = name();
    if (!first) return std::nullopt;
    else if (!_buf.match_literal(":")) return Ret{first, {Name, std::string(first->view())}};
    else if (const auto member = name())
      return Ret{first, {Name, std::string(first->view()) + ":" + std::string(member->view())}};
    return Ret{first, {WholeArchive, std::string(first->view())}};
  }

  // { (<item> | ;)* }, where item returns false if the next token does not start an item.
  template <typename Item> void braced(Item &&item) {
    expect_literal("{");
    while (!_buf.peek_literal("}") && !_buf.peek<lex::EoF>())
      if (!_buf.match_literal(";") && !item()) throw invalid();
    expect_literal("}");
  }

  // ( (<name> [,])* )
  std::vector<std::string> name_list() {
    expect_literal("(");
    std::vector<std::string> ret;
    while (!_buf.peek_literal(")") && !_buf.peek<lex::EoF>()) {
      if (_buf.match_literal(",")) continue;
      const auto item = name();
      if (!item) throw missing(NullaryError::Name_Expected);
      ret.emplace_back(item->view());
    }
    expect_literal(")");
    return ret;
  }

  // ( <name> )
  Identifier parenthesized_name() {
    expect_literal("(");
    auto ret = name();
    if (!ret) throw missing(NullaryError::Name_Expected);
    expect_literal(")");
    return ret;
  }

  // Parse an expression with the expr sub-parser. Returns nullopt if no expression starts at the cursor.
  std::optional<expr::Tree> expression() {
    const expr::NameLocationCounter dot = [] { return std::string("."); };
    _lexer.skip_comments();
    const auto result = expr::parse_nested(_buf, _lexer, _pool, _options, dot);
    if (!result) throw ParserError(UnaryError::Expression_Invalid, result.error().message(), result.error().location);
    else if (!*result) return std::nullopt;
    return (*result)->tree;
  }

  // Match a name, either quoted or not. Returns nullptr if the next token is not a name.
  Identifier name() {
    return std::static_pointer_cast<lex::Identifier>(_buf.match(lex::Identifier::TYPE | lex::StringConstant::TYPE));
  }
  // Match an unquoted identifier whose text satisfies matches. Returns nullptr otherwise.
  template <typename Predicate> Identifier match_keyword(Predicate &&matches) {
    const auto identifier = _buf.peek<lex::Identifier>();
    if (!identifier || !matches(identifier->view())) return nullptr;
    return _buf.match<lex::Identifier>();
  }

  // Interval spanning from first to the last matched token.
  LocationInterval matched_since(const TokenPtr &first) const {
    return LocationInterval(first->location().lower(), _buf.matched_tokens().back()->location().upper());
  }
  // Match literal, or report it as missing at the next token.
  void expect_literal(const std::string &literal) {
    if (!_buf.match_literal(literal)) throw ParserError(UnaryError::Token_Expected, literal, _buf.peek()->location());
  }
  // Create an error for a missing construct, located at the next token.
  ParserError missing(NullaryError error) { return ParserError(error, _buf.peek()->location()); }
  ParserError invalid() {
    const auto next = _buf.peek();
    return ParserError(UnaryError::Token_Invalid, next->repr(), next->location());
  }

  // Skip to the end of the command that failed, so that parsing resumes at the next command. A command ends at a ; or
  // at the } that closes it.
  void synchronize(const lex::Marker &start) {
    // +1 if {, -1 if }, else 0.
    const auto nesting = [](const TokenPtr &token) { return (literal_of(token) == "{") - (literal_of(token) == "}"); };
    int depth = 0;
    for (const auto &token : _buf.matched_tokens_after(start)) depth += nesting(token);
    // Consume tokens until we reach out initial depth.
    while (const auto token = _buf.match_not<lex::EoF>()) {
      depth += nesting(token);
      if (depth <= 0 && (literal_of(token) == ";" || literal_of(token) == "}")) return;
    }
  }

  pepp::tc::DiagnosticTable &_diag;
  std::shared_ptr<expr::IdentifierPool> _pool = std::make_shared<expr::IdentifierPool>();
  Uncommented _lexer;
  lex::Buffer _buf;
  const expr::Options _options = expression_options();
};
} // namespace

pepp::tc::ldscript::Script pepp::tc::ldscript::parse(DiagnosticTable &diag, std::string_view text,
                                                     support::Location origin) {
  return Parser(diag, support::SeekableData(std::string(text), origin)).script();
}
