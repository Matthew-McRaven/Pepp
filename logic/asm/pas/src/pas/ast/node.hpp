#pragma once

#include "./generic/attr_type.hpp"
#include "./op.hpp"
#include <QMetaType>
#include <QtCore>

namespace pas::ast {
class Node {
public:
  explicit Node(const pas::ast::generic::Type type,
                QWeakPointer<Node> parent = {});
  template <typename T> void set(T attribute);
  template <typename T> const T get() const;
  template <typename T> bool has() const;
  // Recursive
  void apply(ops::ConstOp<bool> &predicate,
             ops::ConstOp<void> &transform) const;
  // Recursive
  void apply(ops::ConstOp<bool> &predicate, ops::MutatingOp<void> &transform);
  // This only
  bool apply(ops::ConstOp<bool> &predicate);

private:
  QVariantMap _attributes;
};

template <typename T> bool Node::has() const {
  return _attributes.contains(T::attributeName);
}

template <typename T> const T Node::get() const {
  QVariant attribute = _attributes[T::attributeName];
  if (attribute.userType() != qMetaTypeId<T>())
    throw std::logic_error("Cannot convert");
  return attribute.value<T>();
}

template <typename T> void Node::set(T attribute) {
  // static_assert(QVariant::fromValue(attribute));
  _attributes[T::attributeName] = QVariant::fromValue(attribute);
}

QWeakPointer<const Node> parent(const Node &node);
QWeakPointer<Node> parent(Node &node);
void setParent(Node &node, QWeakPointer<Node> parent);
const generic::Type type(const Node &node);
// TODO: add custom iterator so that I can have QSharedPointer<const Node>
// override;
QList<QSharedPointer<Node>> children(const Node &node);
QList<QSharedPointer<Node>> children(Node &node);
void addChild(Node &parent, QSharedPointer<Node> child);
} // namespace pas::ast
