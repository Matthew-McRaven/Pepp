#pragma once
#include "pat/ast/argument/character.hpp"
#include "pat/ast/argument/decimal.hpp"
#include "pat/ast/argument/hexadecimal.hpp"
#include "pat/ast/argument/identifier.hpp"
#include "pat/ast/argument/string.hpp"
#include "pat/ast/argument/symbolic.hpp"
#include "pat/bits/strings.hpp"
#include "pat/pep/parse/types_values.hpp"
#include "symbol/table.hpp"
#include <QtCore>
#include <boost/variant/static_visitor.hpp>
namespace pat::ast::argument {
class Base;
};

namespace pat::pep::ast::visitor {
struct ParseToArg
    : public boost::static_visitor<QSharedPointer<pat::ast::argument::Base>> {
  bool preferIdent = false;             // in
  QSharedPointer<symbol::Table> symTab; // in

  std::optional<QString> error = std::nullopt; // out
  QSharedPointer<pat::ast::argument::Base>
  operator()(const pep::parse::CharacterLiteral &);
  QSharedPointer<pat::ast::argument::Base>
  operator()(const pep::parse::StringLiteral &);
  QSharedPointer<pat::ast::argument::Base>
  operator()(const pep::parse::Identifier &);
  QSharedPointer<pat::ast::argument::Base>
  operator()(const pep::parse::DecimalLiteral &);
  QSharedPointer<pat::ast::argument::Base>
  operator()(const pep::parse::HexadecimalLiteral &);
};

QSharedPointer<pat::ast::argument::Base>
ParseToArg::operator()(const pep::parse::CharacterLiteral &line) {
  return QSharedPointer<pat::ast::argument::Character>::create(
      QString::fromStdString(line.value));
}

QSharedPointer<pat::ast::argument::Base>
ParseToArg::operator()(const pep::parse::StringLiteral &line) {
  auto asQString = QString::fromStdString(line.value);
  if (auto length = bits::escapedStringLength(asQString); length <= 2) {
    return QSharedPointer<pat::ast::argument::ShortString>::create(
        asQString, length, bits::BitOrder::BigEndian);
  } else
    return QSharedPointer<pat::ast::argument::LongString>::create(
        asQString, bits::BitOrder::BigEndian);
}
QSharedPointer<pat::ast::argument::Base>
ParseToArg::operator()(const pep::parse::Identifier &line) {
  auto asQString = QString::fromStdString(line.value);
  if (preferIdent)
    return QSharedPointer<pat::ast::argument::Identifier>::create(asQString);
  else {
    auto asSymbol = symTab->reference(asQString);
    return QSharedPointer<pat::ast::argument::Symbolic>::create(
        asSymbol, bits::BitOrder::BigEndian);
  }
}
QSharedPointer<pat::ast::argument::Base>
ParseToArg::operator()(const pep::parse::DecimalLiteral &line) {
  if (line.isSigned)
    return QSharedPointer<pat::ast::argument::SignedDecimal>::create(
        line.value, 2, bits::BitOrder::BigEndian);
  else
    return QSharedPointer<pat::ast::argument::UnsignedDecimal>::create(
        line.value, 2, bits::BitOrder::BigEndian);
}
QSharedPointer<pat::ast::argument::Base>
ParseToArg::operator()(const pep::parse::HexadecimalLiteral &line) {
  return QSharedPointer<pat::ast::argument::Hexadecimal>::create(
      line.value, 2, bits::BitOrder::BigEndian);
}
} // namespace pat::pep::ast::visitor
