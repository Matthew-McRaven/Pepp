#pragma once
#include "pas/parse/pepp/types_values.hpp"
#include "symbol/table.hpp"
#include <QtCore>
#include <boost/variant/static_visitor.hpp>
#include "pas/pas_globals.hpp"

namespace pas::ast::value {
class Base;
};

namespace pas::parse::pepp {
struct PAS_EXPORT ParseToArg
    : public boost::static_visitor<QSharedPointer<pas::ast::value::Base>> {
  bool preferIdent = false;             // in
  QSharedPointer<symbol::Table> symTab; // in
  std::optional<quint8> sizeOverride = std::nullopt;
  QSharedPointer<pas::ast::value::Base> operator()(const CharacterLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const StringLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const Identifier &);
  QSharedPointer<pas::ast::value::Base> operator()(const DecimalLiteral &);
  QSharedPointer<pas::ast::value::Base> operator()(const HexadecimalLiteral &);
};
} // namespace pas::parse::pepp
