#include "./size.hpp"

quint16 pas::ops::pepp::detail::sizeAlign(const ast::Node node, quint16 at,
                                          Direction direction) {
  auto argument = node.get<ast::generic::Argument>().value;
  quint16 align = 0;
  argument->value(reinterpret_cast<quint8 *>(&align), 2, bits::hostOrder());
  if (direction == Direction::Forward)
    return (align - (at % align)) % align;
  else if (direction == Direction::Backward)
    return at % align;
  else
    return 0;
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
  argument->value(reinterpret_cast<quint8 *>(&ret), 2, bits::hostOrder());
  return ret;
}

quint16 pas::ops::pepp::detail::sizeByte(const ast::Node, quint16, Direction) {
  return 1;
}

quint16 pas::ops::pepp::detail::sizeWord(const ast::Node, quint16, Direction) {
  return 2;
}
