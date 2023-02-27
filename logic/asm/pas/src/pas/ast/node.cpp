#include "./node.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/ast/generic/attr_parent.hpp"
#include "pas/ast/generic/attr_type.hpp"

pas::ast::Node::Node(const pas::ast::generic::Type type,
                     QWeakPointer<Node> parent) {
  set(type);
  set<generic::Parent>({.value = parent});
  set<generic::Children>({});
}

QWeakPointer<const pas::ast::Node> pas::ast::parent(const Node &node) {
  return node.get<generic::Parent>().value;
}

QWeakPointer<pas::ast::Node> pas::ast::parent(Node &node) {
  return node.get<generic::Parent>().value;
}

void pas::ast::setParent(Node &node, QWeakPointer<Node> parent) {
  node.set<generic::Parent>({.value = parent});
}

const pas::ast::generic::Type pas::ast::type(const Node &node) {
  return node.get<generic::Type>();
}

QList<QSharedPointer<pas::ast::Node>> pas::ast::children(Node &node) {
  return node.get<generic::Children>().value;
}

QList<QSharedPointer<pas::ast::Node>> pas::ast::children(const Node &node) {
  return node.get<generic::Children>().value;
}
void pas::ast::addChild(Node &parent, QSharedPointer<Node> child) {
  auto childList = children(parent);
  childList.append(child);
  parent.set(generic::Children{.value = childList});
}

QSharedPointer<pas::ast::Node> pas::ast::addError(QSharedPointer<Node> node,
                                                  generic::Message msg) {
  QList<generic::Message> messages;
  if (node->has<generic::Error>())
    messages = node->get<generic::Error>().value;
  messages.push_back(msg);
  node->set(generic::Error{.value = messages});
  return node;
}
