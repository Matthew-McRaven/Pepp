#include "./size.hpp"
#include "pas/ast/generic/attr_address.hpp"
#include "pas/operations/pepp/assign_addr.hpp"

quint16 pas::ops::pepp::detail::sizeAlign(const ast::Node node, quint16 at,
                                          Direction direction) {
  auto argument = node.get<ast::generic::Argument>().value;
  quint16 align = 0;
  argument->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&align), 2},
                  bits::hostOrder());
  quint16 ret = 0;
  if (direction == Direction::Forward)
    ret = (align - (at % align)) % align;
  else if (direction == Direction::Backward)
    ret = at % align;
  else
    ret = 0;
  return ret;
}

quint16 pas::ops::pepp::detail::sizeASCII(const ast::Node node, quint16,
                                          Direction) {
  auto argument = node.get<ast::generic::Argument>().value;
  return argument->size();
}

quint16 pas::ops::pepp::detail::sizeBlock(const ast::Node node, quint16,
                                          Direction) {
  auto argument = node.get<ast::generic::Argument>().value;
  quint16 ret = 0;
  argument->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&ret), 2},
                  bits::hostOrder());
  return ret;
}

quint16 pas::ops::pepp::detail::sizeByte(const ast::Node, quint16, Direction) {
  return 1;
}

quint16 pas::ops::pepp::detail::sizeWord(const ast::Node, quint16, Direction) {
  return 2;
}

pas::ops::pepp::Direction pas::ops::pepp::direction(const ast::Node &node) {
  auto children = ast::children(node);
  return detail::hasBurn(children) ? Direction::Backward : Direction::Forward;
}

quint16 pas::ops::pepp::baseAddress(const ast::Node &node) {
  if (direction(node) == pas::ops::pepp::Direction::Forward)
    return 0;
  auto children = ast::children(node);
  return detail::getBurnArg(children);
}

qsizetype pas::ops::pepp::sizeFromAddress(const ast::Node &node) {
  qsizetype ret = 0;

  if (node.has<ast::generic::Address>())
    ret += node.get<ast::generic::Address>().value.size;

  for (auto &child : ast::children(node))
    ret += sizeFromAddress(*child);

  return ret;
}
