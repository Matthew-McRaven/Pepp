#include "./pep_parser.hpp"
#include "toolchain/pas/ast/value/character.hpp"
#include "toolchain/pas/ast/value/decimal.hpp"
#include "toolchain/pas/ast/value/numeric.hpp"
#include "toolchain/pas/ast/value/string.hpp"

#include <toolchain/pas/ast/value/hexadecimal.hpp>

#include <toolchain2/lex/asm/pep_tokens.hpp>

pepp::tc::parser::PepParser::PepParser(pepp::tc::support::SeekableData &&data)
    : _pool(std::make_shared<pepp::tc::support::StringPool>()),
      _lexer(std::make_shared<lex::PepLexer>(_pool, std::move(data))), _buffer(std::make_shared<lex::Buffer>(&*_lexer)),
      _symtab(std::make_shared<symbol::Table>(16)) {}

std::vector<std::shared_ptr<pepp::tc::ir::LinearIR>> pepp::tc::parser::PepParser::parse() {
  std::vector<std::shared_ptr<ir::LinearIR>> lines;

  while (_buffer->input_remains()) {
    false;
  }
  return lines;
}

std::shared_ptr<symbol::Table> pepp::tc::parser::PepParser::symbol_table() const { return _symtab; }

std::shared_ptr<pas::ast::value::Base> pepp::tc::parser::PepParser::argument() {
  lex::Checkpoint cp(*_buffer);
  static constexpr auto le = bits::Order::LittleEndian;
  if (auto maybeInteger = _buffer->match<lex::Integer>(); maybeInteger) {
    if (maybeInteger->format == lex::Integer::Format::SignedDec)
      return std::make_shared<pas::ast::value::SignedDecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::Hex)
      return std::make_shared<pas::ast::value::Hexadecimal>(maybeInteger->value, 2);
    else if (maybeInteger->format == lex::Integer::Format::UnsignedDec)
      return std::make_shared<pas::ast::value::UnsignedDecimal>(maybeInteger->value, 2);
    else throw std::logic_error("Unrecognized integer format");
  } else if (auto maybeIdent = _buffer->match<lex::Identifier>(); maybeIdent) {
    auto entry = _symtab->reference(maybeIdent->to_string());
    return std::make_shared<pas::ast::value::Symbolic>(entry);
  } else if (auto maybeChar = _buffer->match<lex::CharacterConstant>(); maybeChar) {
    return std::make_shared<pas::ast::value::Character>(maybeChar->value);
  } else if (auto maybeStr = _buffer->match<lex::StringConstant>(); maybeStr) {
    auto asStr = maybeStr->view().toString();
    if (asStr.size() <= 2) return std::make_shared<pas::ast::value::ShortString>(asStr, asStr.size(), le);
    return std::make_shared<pas::ast::value::LongString>(asStr, le);
  } else return nullptr;
}
