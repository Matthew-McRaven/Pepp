#pragma once
#include "pas/parse/pepp/rules_values.hpp"
#include "symbol/table.hpp"
#include <QtCore>
#include <boost/variant/static_visitor.hpp>
namespace pas::ast::value {
class Base;
};

namespace pas::operations::pepp {
using namespace pas::parse::pepp;
struct ParseToArg
    : public boost::static_visitor<QSharedPointer<pas::ast::value::Base>> {
  bool preferIdent = false;             // in
  QSharedPointer<symbol::Table> symTab; // in
  QSharedPointer<pas::ast::value::Base> operator()(const CharacterLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const StringLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const Identifier &);
  QSharedPointer<pas::ast::value::Base> operator()(const DecimalLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const HexadecimalLiteral &);
};
} // namespace pas::operations::pepp
