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
} // namespace pat::pep::ast::visitor
