#include "./arg_from_parse_tree.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/ast/value/character.hpp"
#include "pas/ast/value/decimal.hpp"
#include "pas/ast/value/hexadecimal.hpp"
#include "pas/ast/value/identifier.hpp"
#include "pas/ast/value/string.hpp"
#include "pas/ast/value/symbolic.hpp"
#include "pas/bits/strings.hpp"
using namespace pas::ast::value;
QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const CharacterLiteral &line) {
  return QSharedPointer<pas::ast::value::Character>::create(
      QString::fromStdString(line.value));
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const StringLiteral &line) {
  auto asQString = QString::fromStdString(line.value);
  if (auto length = pas::bits::escapedStringLength(asQString); length <= 2) {
    return QSharedPointer<pas::ast::value::ShortString>::create(
        asQString, length, bits::BitOrder::BigEndian);
  } else
    return QSharedPointer<pas::ast::value::LongString>::create(
        asQString, bits::BitOrder::BigEndian);
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const Identifier &line) {
  auto asQString = QString::fromStdString(line.value);
  if (preferIdent)
    return QSharedPointer<pas::ast::value::Identifier>::create(asQString);
  else {
    auto asSymbol = symTab->reference(asQString);
    return QSharedPointer<pas::ast::value::Symbolic>::create(asSymbol);
  }
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const DecimalLiteral &line) {
  if (line.isSigned)
    return QSharedPointer<pas::ast::value::SignedDecimal>::create(
        line.value, 2, bits::BitOrder::BigEndian);
  else
    return QSharedPointer<pas::ast::value::UnsignedDecimal>::create(
        line.value, 2, bits::BitOrder::BigEndian);
}

QSharedPointer<pas::ast::value::Base>
pas::parse::pepp::ParseToArg::operator()(const HexadecimalLiteral &line) {
  return QSharedPointer<pas::ast::value::Hexadecimal>::create(
      line.value, 2, bits::BitOrder::BigEndian);
}
