#pragma once
#include "./string.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/generic/string.hpp"
#include "pas/operations/pepp/is.hpp"
#include "symbol/entry.hpp"
#include <QtCore>
#include <pas/ast/generic/attr_comment.hpp>
#include <pas/ast/generic/attr_symbol.hpp>

namespace pas::ops::pepp {

template <typename ISA> struct FormatSource : public pas::ops::ConstOp<void> {
  QStringList ret;
  void operator()(const ast::Node &node) override;
};

template <typename ISA> QStringList formatSource(const ast::Node &node);
namespace detail {
template <typename ISA> QString formatUnary(const ast::Node &node);
template <typename ISA> QString formatNonUnary(const ast::Node &node);
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA>
void pas::ops::pepp::FormatSource<ISA>::operator()(const ast::Node &node) {
  using namespace pas::ops::generic;
  if (generic::isDirective()(node))
    ret.push_back(generic::detail::formatDirective(node));
  else if (generic::isMacro()(node))
    ret.push_back(generic::detail::formatMacro(node));
  else if (generic::isComment()(node))
    ret.push_back(generic::detail::formatComment(node));
  else if (generic::isBlank()(node))
    ret.push_back(generic::detail::formatBlank(node));
  else if (pepp::isUnary<ISA>()(node))
    ret.push_back(pepp::detail::formatUnary<ISA>(node));
  else if (pepp::isNonUnary<ISA>()(node))
    ret.push_back(pepp::detail::formatNonUnary<ISA>(node));
}

template <typename ISA>
QStringList pas::ops::pepp::formatSource(const ast::Node &node) {
  auto visit = FormatSource<ISA>();
  ast::apply_recurse(node, visit);
  return visit.ret;
}

template <typename ISA>
QString pas::ops::pepp::detail::formatUnary(const ast::Node &node) {
  QString symbol = "";
  if (node.has<pas::ast::generic::SymbolDeclaration>())
    symbol = node.get<pas::ast::generic::SymbolDeclaration>().value->name;

  QString instr =
      ISA::string(node.get<pas::ast::pepp::Instruction<ISA>>().value);
  QString comment = "";

  if (node.has<pas::ast::generic::Comment>())
    comment = node.get<pas::ast::generic::Comment>().value;

  return generic::detail::format(symbol, instr, {}, comment);
}

template <typename ISA>
QString pas::ops::pepp::detail::formatNonUnary(const ast::Node &node) {
  QString symbol = "";
  if (node.has<pas::ast::generic::SymbolDeclaration>())
    symbol = node.get<pas::ast::generic::SymbolDeclaration>().value->name;

  auto instr = node.get<pas::ast::pepp::Instruction<ISA>>().value;

  QStringList args;
  args.push_back(node.get<ast::generic::Argument>().value->string());
  auto addr = node.get<pas::ast::pepp::AddressingMode<ISA>>().value;
  if (!ISA::canElideAddressingMode(instr, addr))
    args.push_back(ISA::string(addr));

  QString comment = "";
  if (node.has<pas::ast::generic::Comment>())
    comment = node.get<pas::ast::generic::Comment>().value;

  return generic::detail::format(symbol, ISA::string(instr), args, comment);
}
