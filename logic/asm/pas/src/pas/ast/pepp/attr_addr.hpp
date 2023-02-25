#pragma once
#include <QtCore>

namespace pas::ast::pepp {
template <typename ISA> struct AddressingMode {
  static const inline QString attributeName = u"pepp:addr"_qs;
  typename ISA::AddressingMode value = {};
};
} // namespace pas::ast::pepp
// Must add this to ISA declaration.
// Q_DECLARE_METATYPE(pas::ast::pepp::AddressingMode<T>);
