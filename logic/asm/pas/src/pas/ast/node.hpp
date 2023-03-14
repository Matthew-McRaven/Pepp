#pragma once

#include "./generic/attr_type.hpp"
#include "./op.hpp"
#include "pas/ast/generic/attr_error.hpp"
#include <QMetaType>
#include <QtCore>

namespace pas::ast {
class Node : public QEnableSharedFromThis<Node> {
public:
  explicit Node(const pas::ast::generic::Type type,
                QWeakPointer<Node> parent = {});
  template <typename T> bool has() const;
  template <typename T> const T get() const;
  // Get && remove from attributes.
  template <typename T> T take();
  // do not modify
  const QMap<QString, QVariant> attributes() const;
  void fromAttributes(const QMap<QString, QVariant> attributes);
  template <typename T> void set(T attribute);
  template <typename T> T apply_self(ops::ConstOp<T> &transform) const;
  template <typename T> T apply_self(ops::MutatingOp<T> &transform);
  template <typename T>
  std::optional<T> apply_self(ops::ConstOp<bool> &predicate,
                              ops::ConstOp<T> &transform) const;
  template <typename T>
  std::optional<T> apply_self(ops::ConstOp<bool> &predicate,
                              ops::MutatingOp<T> &transform);

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

template <typename T> T Node::take() {
  QVariant attribute = _attributes[T::attributeName];
  if (attribute.userType() != qMetaTypeId<T>())
    throw std::logic_error("Cannot convert");
  _attributes.remove(T::attributeName);
  return attribute.value<T>();
}

template <typename T> void Node::set(T attribute) {
  // static_assert(QVariant::fromValue(attribute));
  _attributes[T::attributeName] = QVariant::fromValue(attribute);
}

template <typename T> T Node::apply_self(ops::ConstOp<T> &transform) const {
  return transform(*this);
}

template <typename T> T Node::apply_self(ops::MutatingOp<T> &transform) {
  return transform(*this);
}

QWeakPointer<const Node> parent(const Node &node);
QWeakPointer<Node> parent(Node &node);
void setParent(Node &node, QWeakPointer<Node> parent);
const generic::Type type(const Node &node);
// TODO: add custom iterator so that I can have QSharedPointer<const Node>
// override;
QList<QSharedPointer<Node>> children(const Node &node);
QList<QSharedPointer<Node>> children(Node &node);
// Does not update child's parent pointer.
void addChild(Node &parent, QSharedPointer<Node> child);
void setAddress(Node &node, quint64 start, quint64 end);

QSharedPointer<Node> addError(QSharedPointer<Node> node,
                              pas::ast::generic::Message msg);
void addError(Node &node, pas::ast::generic::Message msg);
// Shorthand to reduce template verbosity in calling contexts
template <typename T> bool matches(const Node &node, const T &value) {
  return node.has<T>() && node.get<T>() == value;
}

template <typename T>
std::optional<T> apply_self_if(const Node &node, ops::ConstOp<bool> &predicate,
                               ops::ConstOp<T> &transform) {
  if (!node.apply_self(predicate))
    return std::nullopt;
  return node.apply_self(transform);
}

template <typename T>
std::optional<T> apply_self_if(Node &node, ops::ConstOp<bool> &predicate,
                               ops::MutatingOp<T> &transform) {
  if (!node.apply_self(predicate))
    return std::nullopt;
  return node.apply_self(transform);
}

template <typename T>
std::optional<T> apply_if(Node &node, ops::ConstOp<bool> &predicate,
                          ops::MutatingOp<T> &transform) {
  if (!node.apply_self(predicate))
    return std::nullopt;
  return node.apply_self(transform);
}

// If there is a result, it must be accumulated inside transform.
template <typename T>
void apply_recurse(const Node &node, ops::ConstOp<T> &transform) {
  node.apply_self(transform);
  for (auto &child : children(node))
    apply_recurse(*child, transform);
}

// If there is a result, it must be accumulated inside transform.
template <typename T>
void apply_recurse(Node &node, ops::MutatingOp<T> &transform) {
  node.apply_self(transform);
  for (auto &child : children(node))
    apply_recurse(*child, transform);
}

// If there is a result, it must be accumulated inside transform.
template <typename T>
void apply_recurse_if(const Node &node, ops::ConstOp<bool> &predicate,
                      ops::ConstOp<T> &transform) {
  apply_self_if(node, predicate, transform);
  for (auto &child : children(node))
    apply_recurse_if(*child, predicate, transform);
}

// If there is a result, it must be accumulated inside transform.
template <typename T>
void apply_recurse_if(Node &node, ops::ConstOp<bool> &predicate,
                      ops::MutatingOp<T> &transform) {
  apply_self_if(node, predicate, transform);
  for (auto &child : children(node))
    apply_recurse_if(*child, predicate, transform);
}
} // namespace pas::ast
