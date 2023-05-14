#include "./bytes.hpp"
#include "bits/operations/copy.hpp"
#include "bits/strings.hpp"
#include "pas/ast/generic/attr_address.hpp"
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/value/base.hpp"

quint16 alignToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto address = node.get<pas::ast::generic::Address>().value;
  if (dest.size() < address.size)
    return 0;
  // TODO: replace with bits.
  memset(dest.data(), 0, address.size);
  return address.size;
}

quint16 asciiToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  auto size = argument->size();
  if (dest.size() < size)
    return 0;
  // Strings are always host order, since they are single bytes, not multi-byte.
  // Single bytes have no order.
  argument->value(dest.first(size), bits::hostOrder());
  return size;
}

quint16 blockToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  quint16 size = 0;
  argument->value(
      bits::span<quint8>{reinterpret_cast<quint8 *>(&size), sizeof(size)},
      bits::hostOrder());
  if (dest.size() < size)
    return 0;
  // TODO: replace with bits.
  memset(dest.data(), 0, size);
  return size;
}

quint16 byteToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  auto size = argument->size();
  if (dest.size() < size)
    return 0;
  argument->value(dest.first(size));
  return size;
}

quint16 wordToBytes(const pas::ast::Node &node, bits::span<quint8> dest) {
  auto argument = node.get<pas::ast::generic::Argument>().value;
  auto size = argument->size();
  if (dest.size() < size)
    return 0;
  argument->value(dest.first(size));
  return size;
}

using convertFn =
    std::function<quint16(const pas::ast::Node &, bits::span<quint8>)>;
quint16 pas::ops::pepp::detail::directiveToBytes(const ast::Node &node,
                                                 bits::span<quint8> dest) {
  static QMap<QString, convertFn> directives = {{u"ALIGN"_qs, alignToBytes},
                                                {u"ASCII"_qs, asciiToBytes},
                                                {u"BLOCK"_qs, blockToBytes},
                                                {u"BYTE"_qs, byteToBytes},
                                                {u"WORD"_qs, wordToBytes}};
  QString directive = node.get<ast::generic::Directive>().value;
  if (auto it = directives.find(directive); it != directives.end())
    return it.value()(node, dest);
  else
    return 0;
}

QString pas::ops::pepp::bytesToObject(const QList<quint8> &bytes,
                                      quint8 bytesPerLine) {
  static const char *term = "zz";
  static const bits::span<const char> termSpan = {term, 3};
  auto obj = QList<char>((bytes.size()) * 3 + 2);
  auto objSpan = std::span<char>{obj.data(), std::size_t(obj.size())};
  QList<bits::SeparatorRule> rules = {
      {.skipFirst = true, .separator = '\n', .modulus = bytesPerLine},
      {.skipFirst = false, .separator = ' ', .modulus = 1}};
  auto it = bits::bytesToAsciiHex(
      objSpan, {bytes.constData(), static_cast<std::size_t>(bytes.size())},
      rules);

  bits::memcpy(objSpan.subspan(it), termSpan);
  return QString::fromLocal8Bit(obj.data(), obj.size());
}
