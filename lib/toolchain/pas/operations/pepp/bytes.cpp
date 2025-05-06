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

#include "./bytes.hpp"
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "utils/bits/copy.hpp"
#include "utils/bits/strings.hpp"

quint16 alignToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto address = node.get<pas::ast::generic::Address>().value;
  if (dest.size() < address.size) return 0;
  // TODO: replace with bits.
  memset(dest.data(), 0, address.size);
  return address.size;
}

quint16 asciiToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  auto size = argument->size();
  if (dest.size() < size) return 0;
  // Strings are always host order, since they are single bytes, not multi-byte.
  // Single bytes have no order.
  argument->value(dest.first(size), bits::hostOrder());
  return size;
}

quint16 blockToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  quint16 size = 0;
  argument->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&size), sizeof(size)}, bits::hostOrder());
  if (dest.size() < size) return 0;
  // TODO: replace with bits.
  memset(dest.data(), 0, size);
  return size;
}

quint16 byteToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  auto size = argument->size();
  if (dest.size() < size) return 0;
  argument->value(dest.first(size));
  return size;
}

quint16 wordToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  auto size = argument->size();
  if (dest.size() < size) return 0;
  argument->value(dest.first(size));
  return size;
}

using convertFn = std::function<quint16(const pas::ast::Node &, bits::span<quint8>)>;
quint16 pas::ops::pepp::detail::directiveToBytes(const ast::Node &node, bits::span<quint8> dest) {
  static QMap<QString, convertFn> directives = {{"ADDRSS", wordToBytes}, {"ALIGN", alignToBytes},
                                                {"ASCII", asciiToBytes}, {"BLOCK", blockToBytes},
                                                {"BYTE", byteToBytes},   {"WORD", wordToBytes}};
  QString directive = node.get<ast::generic::Directive>().value;
  if (auto it = directives.find(directive); it != directives.end()) return it.value()(node, dest);
  else return 0;
}

QString pas::ops::pepp::bytesToObject(const QList<quint8> &bytes, quint8 bytesPerLine) {
  static const char *term = "zz";
  static const bits::span<const char> termSpan = {term, 3};
  auto obj = QList<char>((bytes.size()) * 3 + 2);
  auto objSpan = std::span<char>{obj.data(), std::size_t(obj.size())};
  QList<bits::SeparatorRule> rules = {{.skipFirst = true, .separator = '\n', .modulus = bytesPerLine},
                                      {.skipFirst = false, .separator = ' ', .modulus = 1}};
  auto it = bits::bytesToAsciiHex(objSpan, {bytes.constData(), static_cast<std::size_t>(bytes.size())}, rules);

  bits::memcpy(objSpan.subspan(it), termSpan);
  return QString::fromLocal8Bit(obj.data(), obj.size());
}
