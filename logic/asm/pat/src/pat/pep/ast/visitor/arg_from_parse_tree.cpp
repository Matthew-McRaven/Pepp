#include "./arg_from_parse_tree.hpp"

QSharedPointer<pat::ast::argument::Base> pat::pep::ast::visitor::ParseToArg::operator()(const parse::CharacterLiteral &line) {
  return QSharedPointer<pat::ast::argument::Character>::create(
      QString::fromStdString(line.value));
}

QSharedPointer<pat::ast::argument::Base> pat::pep::ast::visitor::ParseToArg::operator()(const parse::StringLiteral &line) {
  auto asQString = QString::fromStdString(line.value);
  if (auto length = bits::escapedStringLength(asQString); length <= 2) {
    return QSharedPointer<pat::ast::argument::ShortString>::create(
        asQString, length, bits::BitOrder::BigEndian);
  } else
    return QSharedPointer<pat::ast::argument::LongString>::create(
        asQString, bits::BitOrder::BigEndian);
}

QSharedPointer<pat::ast::argument::Base> pat::pep::ast::visitor::ParseToArg::operator()(const parse::Identifier &line) {
  auto asQString = QString::fromStdString(line.value);
  if (preferIdent)
    return QSharedPointer<pat::ast::argument::Identifier>::create(asQString);
  else {
    auto asSymbol = symTab->reference(asQString);
    return QSharedPointer<pat::ast::argument::Symbolic>::create(
        asSymbol, bits::BitOrder::BigEndian);
  }
}

QSharedPointer<pat::ast::argument::Base> pat::pep::ast::visitor::ParseToArg::operator()(const parse::DecimalLiteral &line) {
  if (line.isSigned)
    return QSharedPointer<pat::ast::argument::SignedDecimal>::create(
        line.value, 2, bits::BitOrder::BigEndian);
  else
    return QSharedPointer<pat::ast::argument::UnsignedDecimal>::create(
        line.value, 2, bits::BitOrder::BigEndian);
}

QSharedPointer<pat::ast::argument::Base> pat::pep::ast::visitor::ParseToArg::operator()(const parse::HexadecimalLiteral &line) {
  return QSharedPointer<pat::ast::argument::Hexadecimal>::create(
      line.value, 2, bits::BitOrder::BigEndian);
}
