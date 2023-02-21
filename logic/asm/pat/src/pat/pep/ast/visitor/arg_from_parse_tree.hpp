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

namespace pat::pep::ast::visitors {
struct ParseToArg : public boost::static_visitor<pat::ast::argument::Base *> {
  bool preferIdent = false;             // in
  QSharedPointer<symbol::Table> symTab; // in

  std::optional<QString> error = std::nullopt; // out
  pat::ast::argument::Base *operator()(const pep::parse::CharacterLiteral &);
  pat::ast::argument::Base *operator()(const pep::parse::StringLiteral &);
  pat::ast::argument::Base *operator()(const pep::parse::Identifier &);
  pat::ast::argument::Base *operator()(const pep::parse::DecimalLiteral &);
  pat::ast::argument::Base *operator()(const pep::parse::HexadecimalLiteral &);
};

pat::ast::argument::Base *
ParseToArg::operator()(const pep::parse::CharacterLiteral &line) {
  return new pat::ast::argument::Character(QString::fromStdString(line.value));
}

pat::ast::argument::Base *
ParseToArg::operator()(const pep::parse::StringLiteral &line) {
  auto asQString = QString::fromStdString(line.value);
  if (auto length = bits::escapedStringLength(asQString); length <= 2) {
    return new pat::ast::argument::ShortString(asQString, length,
                                               bits::BitOrder::BigEndian);
  } else
    return new pat::ast::argument::LongString(asQString,
                                              bits::BitOrder::BigEndian);
}
pat::ast::argument::Base *
ParseToArg::operator()(const pep::parse::Identifier &line) {
  auto asQString = QString::fromStdString(line.value);
  if (preferIdent)
    return new pat::ast::argument::Identifier(asQString);
  else {
    auto asSymbol = symTab->reference(asQString);
    return new pat::ast::argument::Symbolic(asSymbol,
                                            bits::BitOrder::BigEndian);
  }
}
pat::ast::argument::Base *
ParseToArg::operator()(const pep::parse::DecimalLiteral &line) {
  if (line.isSigned)
    return new pat::ast::argument::SignedDecimal(line.value, 2,
                                                 bits::BitOrder::BigEndian);
  else
    return new pat::ast::argument::UnsignedDecimal(line.value, 2,
                                                   bits::BitOrder::BigEndian);
}
pat::ast::argument::Base *
ParseToArg::operator()(const pep::parse::HexadecimalLiteral &line) {
  return new pat::ast::argument::Hexadecimal(line.value, 2,
                                             bits::BitOrder::BigEndian);
}
} // namespace pat::pep::ast::visitors
