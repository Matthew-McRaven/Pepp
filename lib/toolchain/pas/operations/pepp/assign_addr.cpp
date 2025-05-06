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

#include "./assign_addr.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "is.hpp"

bool pas::ops::pepp::detail::hasBurn(QList<QSharedPointer<pas::ast::Node>> &list) {
  for (auto &child : list)
    if (pas::ops::pepp::isBurn()(*child)) return true;
  return false;
}

quint16 pas::ops::pepp::detail::getBurnArg(QList<QSharedPointer<pas::ast::Node>> &list) {
  quint16 ret = 0xFFFF;
  for (auto &child : list)
    if (pas::ops::pepp::isBurn()(*child)) {
      if (!child->has<pas::ast::generic::Argument>()) return ret;
      else {
        child->get<pas::ast::generic::Argument>().value->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&ret), 2},
                                                               bits::hostOrder());
        return ret;
      }
    }
  return ret;
}

template <> void pas::ops::pepp::assignAddresses<isa::Pep9>(ast::Node &root) {
  auto children = pas::ast::children(root);
  bool isOS = detail::hasBurn(children);
  quint16 base = isOS ? detail::getBurnArg(children) : 0;
  if (isOS)
    for (auto child = children.rbegin(); child != children.rend(); ++child)
      detail::assignAddressesImpl<isa::Pep9>(**child, base, Direction::Backward);
  else
    for (auto &child : children) detail::assignAddressesImpl<isa::Pep9>(*child, base, Direction::Forward);
}
