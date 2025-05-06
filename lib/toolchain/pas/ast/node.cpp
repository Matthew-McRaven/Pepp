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

#include "./node.hpp"
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/ast/generic/attr_parent.hpp"
#include "toolchain/pas/ast/generic/attr_type.hpp"

pas::ast::Node::Node(const pas::ast::generic::Type type, QWeakPointer<Node> parent) {
  set(type);
  set<generic::Parent>({.value = parent});
  set<generic::Children>({});
}

const QMap<uint8_t, QVariant> pas::ast::Node::attributes() const {
  auto ret = _attributes;
  // Attempt to prevent common data from being modified
  ret.detach();
  return ret;
}

void pas::ast::Node::fromAttributes(const QMap<uint8_t, QVariant> attributes) {
  for (auto key = attributes.keyBegin(); key != attributes.keyEnd(); ++key) {
    _attributes[*key] = attributes[*key];
  }
}

QWeakPointer<const pas::ast::Node> pas::ast::parent(const Node &node) { return node.get<generic::Parent>().value; }

QWeakPointer<pas::ast::Node> pas::ast::parent(Node &node) { return node.get<generic::Parent>().value; }

void pas::ast::setParent(Node &node, QWeakPointer<Node> parent) { node.set<generic::Parent>({.value = parent}); }

const pas::ast::generic::Type pas::ast::type(const Node &node) { return node.get<generic::Type>(); }

QList<QSharedPointer<pas::ast::Node>> pas::ast::children(Node &node) { return node.get<generic::Children>().value; }

QList<QSharedPointer<pas::ast::Node>> pas::ast::children(const Node &node) {
  return node.get<generic::Children>().value;
}
void pas::ast::addChild(Node &parent, QSharedPointer<Node> child) {
  auto childList = children(parent);
  childList.append(child);
  parent.set(generic::Children{.value = childList});
}

QSharedPointer<pas::ast::Node> pas::ast::addError(QSharedPointer<Node> node, generic::Message msg) {
  QList<generic::Message> messages;
  if (node->has<generic::Error>()) messages = node->get<generic::Error>().value;
  messages.push_back(msg);
  node->set(generic::Error{.value = messages});
  return node;
}

void pas::ast::addError(Node &node, generic::Message msg) {
  QList<generic::Message> messages;
  if (node.has<generic::Error>()) messages = node.get<generic::Error>().value;
  messages.push_back(msg);
  node.set(generic::Error{.value = messages});
}

void pas::ast::setAddress(Node &node, quint64 start, quint64 size) {
  node.set(generic::Address{.value = {.start = start, .size = size}});
}
