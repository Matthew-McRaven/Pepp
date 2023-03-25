#pragma once
#include "./string.hpp"
#include "pas/ast/generic/attr_address.hpp"
#include "pas/ast/generic/attr_comment.hpp"
#include "pas/ast/generic/attr_hide.hpp"
#include "pas/ast/generic/attr_symbol.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/generic/string.hpp"
#include "pas/operations/pepp/bytes.hpp"
#include "pas/operations/pepp/is.hpp"
#include "pas/operations/pepp/size.hpp"
#include "symbol/entry.hpp"
#include <QtCore>

namespace pas::ops::pepp {

// Non-recursively format a single node as source.
template <typename ISA> QString format(const ast::Node &node);
// Non-recursively format a signle node as listing.
template <typename ISA>
QStringList list(const ast::Node &node, quint16 bytesPerLine);

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatSource : public pas::ops::ConstOp<void> {
  QStringList ret;
  void operator()(const ast::Node &node) override;
};

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatListing : public pas::ops::ConstOp<void> {
  QStringList ret;
  quint8 bytesPerLine = 3;
  void operator()(const ast::Node &node) override;
};

// Format entire tree recursively as source.
template <typename ISA> QStringList formatSource(const ast::Node &node);
// Format entire tree recursively as listing.
template <typename ISA>
QStringList formatListing(const ast::Node &node, quint16 bytesPerLine = 3);


namespace detail {
// Format single unary node as source.
template <typename ISA> QString formatUnary(const ast::Node &node);
// Format single non-unary node as source.
template <typename ISA> QString formatNonUnary(const ast::Node &node);
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA> QString pas::ops::pepp::format(const ast::Node &node) {
  using namespace pas::ops::generic;
  if (generic::isDirective()(node))
    return generic::detail::formatDirective(node);
  else if (generic::isMacro()(node))
    return generic::detail::formatMacro(node);
  else if (generic::isComment()(node))
    return generic::detail::formatComment(node);
  else if (generic::isBlank()(node))
    return generic::detail::formatBlank(node);
  else if (pepp::isUnary<ISA>()(node))
    return pepp::detail::formatUnary<ISA>(node);
  else if (pepp::isNonUnary<ISA>()(node))
    return pepp::detail::formatNonUnary<ISA>(node);
  else
    return "";
}

template <typename ISA>
QStringList pas::ops::pepp::list(const pas::ast::Node &node,
                                 quint16 bytesPerLine) {
  auto type = node.get<ast::generic::Type>().value;
  if (type == ast::generic::Type::Structural)
    return {};
  QStringList ret;
  QList<quint8> bytes = {};
  const auto byteCharCount = 2*bytesPerLine;
  // If the node wants to hide object code, leave the bytes empty.
  // If the node has no address, then it can emit no bytes
  if ((!node.has<ast::generic::Hide>() ||
       node.get<ast::generic::Hide>().value.object ==
           ast::generic::Hide::In::Object::Emit) &&
      node.has<ast::generic::Address>())
    bytes = toBytes<ISA>(node);

  QString address;
  if (node.has<ast::generic::Address>())
    address = u"%1"_qs.arg(
      QString::number(node.get<ast::generic::Address>().value.start), 4, '0').toUpper();

  quint16 bytesEmitted = 0;
  QString prettyBytes = "";

  // Accumulate the first row's worth of object code bytes.
  while (bytesEmitted < bytesPerLine && bytesEmitted < bytes.size())
    prettyBytes +=
      u"%1"_qs.arg(QString::number(bytes[bytesEmitted++], 16), 2, QChar('0')).toUpper();

  auto tempString = u"%1 %2 %3"_qs.arg(address, 6).arg(prettyBytes, byteCharCount).arg(format<ISA>(node));
  // Perform right-strip of string. `QString::trimmed() const` trims both ends.
  qsizetype lastIndex = tempString.size()-1;
  while(QChar(tempString[lastIndex]).isSpace() && lastIndex > 0) lastIndex--;
  // If line is all spaces, then the string should be empty. Otherwise, we need to add 1 to last index to convert index (0-based) to size (1-based).
  ret.push_back(lastIndex==0 ? u""_qs : tempString.left(lastIndex+1));
  // pretty bytes have been printed, so we can clear this accumulator value for the next line.
  prettyBytes = "";

  // Emit remaining object code bytes on their own lines.
  while (bytesEmitted < bytesPerLine && bytesEmitted < bytes.size()) {
    prettyBytes +=
      u"%1"_qs.arg(QString::number(bytes[bytesEmitted++], 16), 2, QChar('0')).toUpper();
    if (bytesEmitted % bytesPerLine == 0) {
      ret.push_front(u"%1 %2"_qs.arg("", 4).arg(prettyBytes, byteCharCount));
      prettyBytes="";
    }
  }

  // Handle any bytes in excess of % bytesPerLine.
  if (prettyBytes.size() > 0)
    ret.push_back(u"%1 %2"_qs.arg("",4).arg(prettyBytes, byteCharCount));
  return ret;
}

template <typename ISA>
void pas::ops::pepp::FormatSource<ISA>::operator()(const ast::Node &node) {
  ret.push_back(format<ISA>(node));
}

template <typename ISA>
void pas::ops::pepp::FormatListing<ISA>::operator()(const ast::Node &node) {
  for (auto &line : list<ISA>(node, bytesPerLine))
    ret.push_back(line);
}

template <typename ISA>
QStringList pas::ops::pepp::formatSource(const ast::Node &node) {
  auto visit = FormatSource<ISA>();
  // Do not visit structural nodes, because this will inject unneeded newlines.
  auto is = generic::Negate<generic::isStructural>();
  ast::apply_recurse_if(node, is, visit);
  return visit.ret;
}

template <typename ISA>
QStringList pas::ops::pepp::formatListing(const ast::Node &node,
                                          quint16 bytesPerLine) {
  auto visit = FormatListing<ISA>();
  visit.bytesPerLine = bytesPerLine;
  // Do not visit structural nodes, because this will inject unneeded newlines.
  // Do not visit macro nodes, otherwise macro invocation AND macro body will be
  // printed. At this point, macros should not exist anyways.
  auto is =
      generic::Negate<generic::Or<generic::isStructural, generic::isMacro>>();
  ast::apply_recurse_if(node, is, visit);
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
