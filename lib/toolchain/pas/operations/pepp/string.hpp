/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include <QtCore>
#include "./string.hpp"
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/ast/generic/attr_comment.hpp"
#include "toolchain/pas/ast/generic/attr_hide.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/op.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/generic/string.hpp"
#include "toolchain/pas/operations/pepp/bytes.hpp"
#include "toolchain/pas/operations/pepp/is.hpp"
#include "toolchain/pas/operations/pepp/size.hpp"
#include "toolchain/symbol/entry.hpp"

namespace pas::ops::pepp {

// Non-recursively format a single node as source.
template <typename ISA> QString format(const ast::Node &node, SourceOptions opts = {});

// Non-recursively format a signle node as listing.
template <typename ISA> QStringList list(const ast::Node &node, ListingOptions opts = {});

// Non-recursively format a signle node as `peph`
template <typename ISA> QStringList hexList(const ast::Node &node);

// Non-recursively format a signle node as `pepb`
template <typename ISA> QStringList binList(const ast::Node &node);

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatSource : public pas::ops::ConstOp<void> {
  QStringList ret;
  SourceOptions opts;
  void operator()(const ast::Node &node) override;
};

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatListing : public pas::ops::ConstOp<void> {
  QStringList ret;
  ListingOptions opts;
  void operator()(const ast::Node &node) override;
};

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatSplitListing : public pas::ops::ConstOp<void> {
  QList<QPair<QString, QString>> ret;
  ListingOptions opts;
  void operator()(const ast::Node &node) override;
};

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatHexListing : public pas::ops::ConstOp<void> {
  QStringList ret;
  void operator()(const ast::Node &node) override;
};

// Helper to format with ast::apply methods.
template <typename ISA> struct FormatBinListing : public pas::ops::ConstOp<void> {
  QStringList ret;
  void operator()(const ast::Node &node) override;
};

// Format entire tree recursively as source.
template <typename ISA> QStringList formatSource(const ast::Node &node, SourceOptions opts = {});

// Format entire tree recursively as listing.
template <typename ISA> QStringList formatListing(const ast::Node &node, ListingOptions opts = {});

// Format the entire tree recursively as a listing, with continuation lines separated.
template <typename ISA>
QList<QPair<QString, QString>> formatSplitListing(const ast::Node &node, ListingOptions opts = {});

// Used to format chapter 4 figures of the form: `0000     D1000F ;Load byte accumulator 'H'`
template <typename ISA> QStringList formatHexListing(const ast::Node &node);

// Used to format chapter 4 figures of the form: `0000     1101 0001 1111 1111 1111 1101`
template <typename ISA> QStringList formatBinListing(const ast::Node &node);

namespace detail {
// Format single unary node as source.
template <typename ISA> QString formatUnary(const ast::Node &node, SourceOptions opts);
// Format single non-unary node as source.
template <typename ISA> QString formatNonUnary(const ast::Node &node, SourceOptions opts);

inline QString int2bin(quint8 v) {
  // Take 8-bit integer and format as "xxxx xxxx" binary string, padded with zeros and a space between nybles.
  return QString::number(v, 2).rightJustified(8, '0').insert(4, ' ');
}
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA> QString pas::ops::pepp::format(const ast::Node &node, SourceOptions opts) {
  using namespace pas::ops::generic;
  if (generic::isDirective()(node)) return generic::detail::formatDirective(node, opts);
  else if (generic::isMacro()(node)) return generic::detail::formatMacro(node, opts);
  else if (generic::isComment()(node)) return generic::detail::formatComment(node, opts);
  else if (generic::isBlank()(node)) return generic::detail::formatBlank(node, opts);
  else if (pepp::isUnary<ISA>()(node)) return pepp::detail::formatUnary<ISA>(node, opts);
  else if (pepp::isNonUnary<ISA>()(node)) return pepp::detail::formatNonUnary<ISA>(node, opts);
  else return "";
}

template <typename ISA> QStringList pas::ops::pepp::list(const pas::ast::Node &node, ListingOptions opts) {
  using namespace Qt::StringLiterals;
  QStringList ret;
  auto type = node.get<ast::generic::Type>().value;
  if (type == ast::generic::Type::Structural) return {};
  else if (type == ast::generic::Type::MacroInvoke) {
    for (const auto &child : children(node)) ret.append(hexList<ISA>(*child));
    return ret;
  }

  QList<quint8> bytes = {};
  const auto byteCharCount = 2 * opts.bytesPerLine;
  // If the node wants to hide object code, leave the bytes empty.
  // If the node has no address, then it can emit no bytes
  if ((!node.has<ast::generic::Hide>() ||
       node.get<ast::generic::Hide>().value.object == ast::generic::Hide::In::Object::Emit) &&
      node.has<ast::generic::Address>())
    bytes = toBytes<ISA>(node);

  QString address;
  if (node.has<ast::generic::Address>() &&
      !(node.has<ast::generic::Hide>() && node.get<ast::generic::Hide>().value.addressInListing))
    address = u"%1"_s.arg(QString::number(node.get<ast::generic::Address>().value.start, 16), 4, '0').toUpper();

  quint16 bytesEmitted = 0;
  QString prettyBytes = "";

  // Accumulate the first row's worth of object code bytes.
  while (bytesEmitted < opts.bytesPerLine && static_cast<qsizetype>(bytesEmitted) < bytes.size())
    prettyBytes += u"%1"_s.arg(QString::number(bytes[bytesEmitted++], 16), 2, QChar('0')).toUpper();

  auto tempString = u"%1  %2 %3"_s.arg(address, 4).arg(prettyBytes, -byteCharCount).arg(format<ISA>(node, opts.source));
  // Perform right-strip of string. `QString::trimmed() const` trims both ends.
  qsizetype lastIndex = tempString.size() - 1;
  while (QChar(tempString[lastIndex]).isSpace() && lastIndex > 0) lastIndex--;
  // If line is all spaces, then the string should be empty. Otherwise, we need
  // to add 1 to last index to convert index (0-based) to size (1-based).
  ret.push_back(lastIndex == 0 ? u""_s : tempString.left(lastIndex + 1));
  // pretty bytes have been printed, so we can clear this accumulator value for
  // the next line.
  prettyBytes = "";

  // Emit remaining object code bytes on their own lines.
  while (static_cast<qsizetype>(bytesEmitted) < bytes.size()) {
    prettyBytes += u"%1"_s.arg(QString::number(bytes[bytesEmitted++], 16), 2, QChar('0')).toUpper();
    if (bytesEmitted % opts.bytesPerLine == 0) {
      ret.push_back(u"%1  %2"_s.arg("", 4).arg(prettyBytes, -byteCharCount));
      prettyBytes = "";
    }
  }

  // Handle any bytes in excess of % bytesPerLine.
  if (prettyBytes.size() > 0) ret.push_back(u"%1  %2"_s.arg("", 4).arg(prettyBytes, -byteCharCount));
  return ret;
}

template <typename ISA> QStringList pas::ops::pepp::hexList(const pas::ast::Node &node) {
  using namespace Qt::StringLiterals;
  const auto bytesPerLine = 3;
  const auto byteCharCount = 6;
  QStringList ret;
  auto type = node.get<ast::generic::Type>().value;
  if (type == ast::generic::Type::Structural) return {};
  else if (type == ast::generic::Type::MacroInvoke) {
    for (const auto &child : children(node)) ret.append(hexList<ISA>(*child));
    return ret;
  }

  QList<quint8> bytes = {};

  // If the node wants to hide object code, leave the bytes empty.
  // If the node has no address, then it can emit no bytes
  if ((!node.has<ast::generic::Hide>() ||
       node.get<ast::generic::Hide>().value.object == ast::generic::Hide::In::Object::Emit) &&
      node.has<ast::generic::Address>())
    bytes = toBytes<ISA>(node);

  QString address;
  if (node.has<ast::generic::Address>() &&
      !(node.has<ast::generic::Hide>() && node.get<ast::generic::Hide>().value.addressInListing))
    address = u"%1"_s.arg(QString::number(node.get<ast::generic::Address>().value.start, 16), 4, '0').toUpper();

  quint16 bytesEmitted = 0;
  QString prettyBytes = "";

  // Accumulate the first row's worth of object code bytes.
  while (bytesEmitted < bytesPerLine && static_cast<qsizetype>(bytesEmitted) < bytes.size())
    prettyBytes += u"%1"_s.arg(QString::number(bytes[bytesEmitted++], 16), 2, QChar('0')).toUpper();
  QString comment;
  if (node.has<pas::ast::generic::Comment>()) comment = node.get<pas::ast::generic::Comment>().value;

  auto tempString =
      u"%1     %2 %3"_s.arg(address, 4).arg(prettyBytes, -byteCharCount).arg((comment.isEmpty() ? "" : ";" + comment));
  // Perform right-strip of string. `QString::trimmed() const` trims both ends.
  qsizetype lastIndex = tempString.size() - 1;
  while (QChar(tempString[lastIndex]).isSpace() && lastIndex > 0) lastIndex--;
  // If line is all spaces, then the string should be empty. Otherwise, we need
  // to add 1 to last index to convert index (0-based) to size (1-based).
  ret.push_back(lastIndex == 0 ? u""_s : tempString.left(lastIndex + 1));
  // pretty bytes have been printed, so we can clear this accumulator value for
  // the next line.
  prettyBytes.clear();

  // Emit remaining object code bytes on their own lines.
  while (static_cast<qsizetype>(bytesEmitted) < bytes.size()) {
    prettyBytes += u"%1"_s.arg(QString::number(bytes[bytesEmitted++], 16), 2, QChar('0')).toUpper();
    if (bytesEmitted % bytesPerLine == 0) {
      ret.push_back(u"%1     %2"_s.arg("", 4).arg(prettyBytes, -byteCharCount));
      prettyBytes.clear();
    }
  }

  // Handle any bytes in excess of % bytesPerLine.
  if (prettyBytes.size() > 0) ret.push_back(u"%1     %2"_s.arg("", 4).arg(prettyBytes, -byteCharCount));
  return ret;
}

template <typename ISA> QStringList pas::ops::pepp::binList(const pas::ast::Node &node) {
  using namespace Qt::StringLiterals;
  const auto bytesPerLine = 3;
  // 8 chars per byte with n-1 spaces
  const auto byteCharCount = 8 * bytesPerLine + (bytesPerLine - 1);
  QStringList ret;

  auto type = node.get<ast::generic::Type>().value;
  if (type == ast::generic::Type::Structural) return {};
  else if (type == ast::generic::Type::MacroInvoke) {
    for (const auto &child : children(node)) ret.append(binList<ISA>(*child));
    return ret;
  }

  QList<quint8> bytes = {};

  // If the node wants to hide object code, leave the bytes empty.
  // If the node has no address, then it can emit no bytes
  if ((!node.has<ast::generic::Hide>() ||
       node.get<ast::generic::Hide>().value.object == ast::generic::Hide::In::Object::Emit) &&
      node.has<ast::generic::Address>())
    bytes = toBytes<ISA>(node);

  QString address;
  if (node.has<ast::generic::Address>() &&
      !(node.has<ast::generic::Hide>() && node.get<ast::generic::Hide>().value.addressInListing))
    address = u"%1"_s.arg(QString::number(node.get<ast::generic::Address>().value.start, 16), 4, '0').toUpper();

  quint16 bytesEmitted = 0;
  QString prettyBytes = "";

  // Accumulate the first row's worth of object code bytes.
  while (bytesEmitted < bytesPerLine && static_cast<qsizetype>(bytesEmitted) < bytes.size())
    prettyBytes += detail::int2bin(bytes[bytesEmitted++]) + " ";

  auto tempString = u"%1     %2"_s.arg(address, 4).arg(prettyBytes, -byteCharCount);
  // Perform right-strip of string. `QString::trimmed() const` trims both ends.
  qsizetype lastIndex = tempString.size() - 1;
  while (QChar(tempString[lastIndex]).isSpace() && lastIndex > 0) lastIndex--;
  // If line is all spaces, then the string should be empty. Otherwise, we need
  // to add 1 to last index to convert index (0-based) to size (1-based).
  ret.push_back(lastIndex == 0 ? u""_s : tempString.left(lastIndex + 1));
  // pretty bytes have been printed, so we can clear this accumulator value for
  // the next line.
  prettyBytes.clear();

  // Emit remaining object code bytes on their own lines.
  while (static_cast<qsizetype>(bytesEmitted) < bytes.size()) {
    prettyBytes += detail::int2bin(bytes[bytesEmitted++]) + " ";
    if (bytesEmitted % bytesPerLine == 0) {
      ret.push_back(u"%1     %2"_s.arg("", 4).arg(prettyBytes, -byteCharCount));
      prettyBytes.clear();
    }
  }

  // Handle any bytes in excess of % bytesPerLine.
  if (prettyBytes.size() > 0) ret.push_back(u"%1     %2"_s.arg("", 4).arg(prettyBytes, -byteCharCount));
  return ret;
}

template <typename ISA> void pas::ops::pepp::FormatSource<ISA>::operator()(const ast::Node &node) {
  ret.push_back(format<ISA>(node, opts));
}

template <typename ISA> void pas::ops::pepp::FormatListing<ISA>::operator()(const ast::Node &node) {
  for (auto &line : list<ISA>(node, opts)) ret.push_back(line);
}

template <typename ISA> void pas::ops::pepp::FormatSplitListing<ISA>::operator()(const ast::Node &node) {
  auto lines = list<ISA>(node, opts);
  switch (lines.length()) {
  case 0: return;
  case 1: return ret.push_back({lines[0], {}});
  default: return ret.push_back({lines[0], lines.mid(1).join("\n")});
  }
}

template <typename ISA> void pas::ops::pepp::FormatHexListing<ISA>::operator()(const ast::Node &node) {
  for (auto &line : hexList<ISA>(node)) ret.push_back(line);
}

template <typename ISA> void pas::ops::pepp::FormatBinListing<ISA>::operator()(const ast::Node &node) {
  for (auto &line : binList<ISA>(node)) ret.push_back(line);
}

template <typename ISA> QStringList pas::ops::pepp::formatSource(const ast::Node &node, SourceOptions opts) {
  auto visit = FormatSource<ISA>();
  visit.opts = opts;
  // Do not visit structural nodes, because this will inject unneeded newlines. Do not visit hidden nodes.
  auto is = generic::And<generic::Negate<generic::isStructural>, generic::Negate<generic::SourceHidden>>();
  ast::apply_recurse_if(node, is, visit);
  return visit.ret;
}

template <typename ISA> QStringList pas::ops::pepp::formatListing(const ast::Node &node, ListingOptions opts) {
  auto visit = FormatListing<ISA>();
  visit.opts = opts;
  // Do not visit structural nodes, because this will inject unneeded
  // newlines. Do not visit macro nodes, otherwise macro invocation AND
  // macro body will be printed. At this point, macros should not exist
  // anyways. Do not print/visit hidden nodes.
  auto is = generic::And<generic::Negate<generic::Or<generic::isStructural, generic::isMacro>>,
                         generic::Negate<generic::ListingHidden>>();
  ast::apply_recurse_if(node, is, visit);
  return visit.ret;
}

template <typename ISA>
QList<QPair<QString, QString>> pas::ops::pepp::formatSplitListing(const ast::Node &node, ListingOptions opts) {
  auto visit = FormatSplitListing<ISA>();
  visit.opts = opts;
  // Do not visit structural nodes, because this will inject unneeded
  // newlines. Do not visit macro nodes, otherwise macro invocation AND
  // macro body will be printed. At this point, macros should not exist
  // anyways. Do not print/visit hidden nodes.
  auto is = generic::And<generic::Negate<generic::Or<generic::isStructural, generic::isMacro>>,
                         generic::Negate<generic::ListingHidden>>();
  ast::apply_recurse_if(node, is, visit);
  return visit.ret;
}

template <typename ISA> QStringList pas::ops::pepp::formatHexListing(const ast::Node &node) {
  auto visit = FormatHexListing<ISA>();
  // Do not visit structural nodes, because this will inject unneeded
  // newlines. Do not visit macro nodes, otherwise macro invocation AND
  // macro body will be printed. Do not visit comment-only for sake of brevity.
  // At this point, macros should not exist anyways. Do not print/visit hidden nodes.
  auto is = generic::And<
      generic::Negate<generic::Or<generic::isComment, generic::Or<generic::isStructural, generic::isMacro>>>,
      generic::Negate<generic::ListingHidden>>();
  ast::apply_recurse_if(node, is, visit);
  return visit.ret;
}

template <typename ISA> QStringList pas::ops::pepp::formatBinListing(const ast::Node &node) {
  auto visit = FormatBinListing<ISA>();
  // Do not visit structural nodes, because this will inject unneeded
  // newlines. Do not visit macro nodes, otherwise macro invocation AND
  // macro body will be printed. Do not visit comment-only for sake of brevity.
  // At this point, macros should not exist anyways. Do not print/visit hidden nodes.
  auto is = generic::And<
      generic::Negate<generic::Or<generic::isComment, generic::Or<generic::isStructural, generic::isMacro>>>,
      generic::Negate<generic::ListingHidden>>();
  ast::apply_recurse_if(node, is, visit);
  return visit.ret;
}

template <typename ISA> QString pas::ops::pepp::detail::formatUnary(const ast::Node &node, SourceOptions opts) {
  QString symbol = "";
  if (node.has<pas::ast::generic::SymbolDeclaration>())
    symbol = node.get<pas::ast::generic::SymbolDeclaration>().value->name;

  QString instr = ISA::string(node.get<pas::ast::pepp::Instruction<ISA>>().value);
  QString comment = "";

  if (node.has<pas::ast::generic::Comment>()) comment = node.get<pas::ast::generic::Comment>().value;

  return generic::detail::format(symbol, instr, {}, comment);
}

template <typename ISA> QString pas::ops::pepp::detail::formatNonUnary(const ast::Node &node, SourceOptions opts) {
  QString symbol = "";
  if (node.has<pas::ast::generic::SymbolDeclaration>())
    symbol = node.get<pas::ast::generic::SymbolDeclaration>().value->name;

  auto instr = node.get<pas::ast::pepp::Instruction<ISA>>().value;

  QStringList args;
  args.push_back(node.get<ast::generic::Argument>().value->string());
  auto addr = node.get<pas::ast::pepp::AddressingMode<ISA>>().value;
  if (!ISA::canElideAddressingMode(instr, addr)) args.push_back(ISA::string(addr));

  QString comment = "";
  if (node.has<pas::ast::generic::Comment>()) comment = node.get<pas::ast::generic::Comment>().value;

  return generic::detail::format(symbol, ISA::string(instr), args, comment, 0, false);
}
