#pragma once
#include "pas/ast/generic/attr_address.hpp"
#include "pas/ast/generic/attr_hide.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"
#include "pas/operations/pepp/size.hpp"
#include <QtCore>

namespace pas::ops::pepp {
QString bytesToObject(const QList<quint8> &bytes, quint8 bytesPerLine = 16);
template <typename ISA> QList<quint8> toBytes(const pas::ast::Node &node);
namespace detail {
template <typename ISA>
quint16 nodeToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
quint16 directiveToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
template <typename ISA>
quint16 unaryToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
template <typename ISA>
quint16 nonUnaryToBytes(const pas::ast::Node &node, bits::span<quint8> dest);
} // namespace detail
} // namespace pas::ops::pepp

template <typename ISA>
QList<quint8> pas::ops::pepp::toBytes(const pas::ast::Node &node) {
  auto size = pas::ops::pepp::sizeFromAddress(node);
  auto bytes = QList<quint8>(size);
  auto bytesSpan = bits::span<quint8>(bytes.data(), bytes.size());
  if (detail::nodeToBytes<ISA>(node, bytesSpan))
    return bytes;
  else
    return {};
}

template <typename ISA>
quint16 pas::ops::pepp::detail::nodeToBytes(const pas::ast::Node &node,
                                            bits::span<quint8> dest) {
  // If line has requested object code to be surpressed, don't emit.
  if (node.has<ast::generic::Hide>() &&
      node.get<ast::generic::Hide>().value.object !=
          ast::generic::Hide::In::Object::Emit)
    return true;
  if (pas::ops::generic::isStructural()(node)) {
    std::size_t skipped = 0;
    for (auto &child : node.get<ast::generic::Children>().value) {
      auto size = nodeToBytes<ISA>(*child, dest.subspan(skipped));
      skipped += size;
    }
    return true;
  } else if (pas::ops::generic::isDirective()(node))
    return detail::directiveToBytes(node, dest);
  else if (pas::ops::pepp::isUnary<ISA>()(node))
    return detail::unaryToBytes<ISA>(node, dest);
  else if (pas::ops::pepp::isNonUnary<ISA>()(node))
    return detail::nonUnaryToBytes<ISA>(node, dest);
  else {
    return 0;
  }
}

template <typename ISA>
quint16 pas::ops::pepp::detail::unaryToBytes(const pas::ast::Node &node,
                                             bits::span<quint8> dest) {
  if (dest.size() < 1)
    return 0;
  typename ISA::Mnemonic mnemonic =
      node.get<pas::ast::pepp::Instruction<ISA>>().value;
  dest[0] = ISA::opcode(mnemonic);
  return 1;
}

template <typename ISA>
quint16 pas::ops::pepp::detail::nonUnaryToBytes(const pas::ast::Node &node,
                                                bits::span<quint8> dest) {
  if (dest.size() < 3)
    return 0;
  typename ISA::Mnemonic mnemonic =
      node.get<pas::ast::pepp::Instruction<ISA>>().value;
  typename ISA::AddressingMode addr =
      node.get<pas::ast::pepp::AddressingMode<ISA>>().value;
  dest[0] = ISA::opcode(mnemonic, addr);
  auto arg = node.get<pas::ast::generic::Argument>().value;
  arg->value(dest.subspan(1, 2), bits::Order::BigEndian);
  return 3;
}
