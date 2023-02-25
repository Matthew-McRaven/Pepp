#pragma once
#include <QtCore>

namespace pas::ast::pepp {
template <typename ISA> struct Instruction {
  static const inline QString attributeName = u"pepp:instr"_qs;
  typename ISA::Mnemonic value = {};
};
} // namespace pas::ast::pepp
// Must add this to ISA declaration.
// Q_DECLARE_METATYPE(pas::ast::pepp::Instruction<T>);
