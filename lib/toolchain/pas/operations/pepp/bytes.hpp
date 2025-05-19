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
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/ast/generic/attr_hide.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/is.hpp"
#include "toolchain/pas/operations/pepp/size.hpp"

namespace pas::ops::pepp {
QString bytesToObject(const QList<quint8> &bytes, quint8 bytesPerLine = 8);
template <typename ISA> QList<quint8> toBytes(const pas::ast::Node &node);
namespace detail {
template <typename ISA> quint16 nodeToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
quint16 directiveToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
template <typename ISA> quint16 unaryToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
template <typename ISA> quint16 nonUnaryToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA> QList<quint8> pas::ops::pepp::toBytes(const pas::ast::Node &node) {
  auto size = pas::ops::pepp::sizeFromAddress(node);
  auto bytes = QList<quint8>(size);
  auto bytesSpan = bits::span<quint8>(bytes.data(), bytes.size());
  if (detail::nodeToBytes<ISA>(node, bytesSpan)) return bytes;
  else return {};
}

template <typename ISA>
quint16 pas::ops::pepp::detail::nodeToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  // If line has requested object code to be surpressed, don't emit.
  if (node.has<ast::generic::Hide>() &&
      node.get<ast::generic::Hide>().value.object != ast::generic::Hide::In::Object::Emit)
    return true;
  if (pas::ops::generic::isStructural()(node)) {
    std::size_t skipped = 0;
    for (auto &child : node.get<ast::generic::Children>().value) {
      auto size = nodeToBytes<ISA>(*child, dest.subspan(skipped));
      skipped += size;
    }
    return true;
  } else if (pas::ops::generic::isDirective()(node)) return detail::directiveToBytes(node, dest);
  else if (pas::ops::pepp::isUnary<ISA>()(node)) return detail::unaryToBytes<ISA>(node, dest);
  else if (pas::ops::pepp::isNonUnary<ISA>()(node)) return detail::nonUnaryToBytes<ISA>(node, dest);
  else {
    return 0;
  }
}

template <typename ISA>
quint16 pas::ops::pepp::detail::unaryToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  if (dest.size() < 1) return 0;
  typename ISA::Mnemonic mnemonic = node.get<pas::ast::pepp::Instruction<ISA>>().value;
  dest[0] = ISA::opcode(mnemonic);
  return 1;
}

template <typename ISA>
quint16 pas::ops::pepp::detail::nonUnaryToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  if (dest.size() < 3) return 0;
  typename ISA::Mnemonic mnemonic = node.get<pas::ast::pepp::Instruction<ISA>>().value;
  typename ISA::AddressingMode addr = node.get<pas::ast::pepp::AddressingMode<ISA>>().value;
  dest[0] = ISA::opcode(mnemonic, addr);
  auto arg = node.get<pas::ast::generic::Argument>().value;
  arg->value(dest.subspan(1, 2), bits::Order::BigEndian);
  return 3;
}
