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

#include "./generic/attr_type.hpp"
#include "./op.hpp"
#include "toolchain/pas/ast/generic/attr_error.hpp"
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
  const QMap<uint8_t, QVariant> attributes() const;
  void fromAttributes(const QMap<uint8_t, QVariant> attributes);
  template <typename T> void set(T attribute);
  template <typename T> T apply_self(ops::ConstOp<T> &transform) const;
  template <typename T> T apply_self(ops::MutatingOp<T> &transform);
  template <typename T>
  std::optional<T> apply_self_if(ops::ConstOp<bool> &predicate,
                                 ops::ConstOp<T> &transform) const;
  template <typename T>
  std::optional<T> apply_self_if(ops::ConstOp<bool> &predicate,
                                 ops::MutatingOp<T> &transform);
  // To be used in recurse, because optional<void> is illegal.
  template <typename T>
  void apply_self_if_void(ops::ConstOp<bool> &predicate,
                          ops::ConstOp<T> &transform) const;
  template <typename T>
  void apply_self_if_void(ops::ConstOp<bool> &predicate,
                          ops::MutatingOp<T> &transform);

private:
  QMap<uint8_t, QVariant> _attributes;
};

template <typename T> bool Node::has() const { return _attributes.contains(T::attribute); }

template <typename T> const T Node::get() const {
  QVariant attribute = _attributes[T::attribute];
  if (attribute.userType() != qMetaTypeId<T>()) {
    static const char *const e = "Cannot convert";
    qCritical(e);
    throw std::logic_error(e);
  }
  return attribute.value<T>();
}

template <typename T> T Node::take() {
  QVariant attribute = _attributes[T::attribute];
  if (attribute.userType() != qMetaTypeId<T>()) {
    static const char *const e = "Cannot convert";
    qCritical(e);
    throw std::logic_error(e);
  }
  _attributes.remove(T::attribute);
  return attribute.value<T>();
}

template <typename T> void Node::set(T attribute) {
  // static_assert(QVariant::fromValue(attribute));
  _attributes[T::attribute] = QVariant::fromValue(attribute);
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
void setAddress(Node &node, quint64 start, quint64 size);

QSharedPointer<Node> addError(QSharedPointer<Node> node, pas::ast::generic::Message msg);
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
void apply_self_if_void(const Node &node, ops::ConstOp<bool> &predicate,
                        ops::ConstOp<T> &transform) {
  if (node.apply_self(predicate))
    node.apply_self(transform);
}

template <typename T>
void apply_self_if_void(Node &node, ops::ConstOp<bool> &predicate,
                        ops::MutatingOp<T> &transform) {
  if (node.apply_self(predicate))
    node.apply_self(transform);
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
  apply_self_if_void(node, predicate, transform);
  for (auto &child : children(node))
    apply_recurse_if(*child, predicate, transform);
}

// If there is a result, it must be accumulated inside transform.
template <typename T>
void apply_recurse_if(Node &node, ops::ConstOp<bool> &predicate,
                      ops::MutatingOp<T> &transform) {
  apply_self_if_void(node, predicate, transform);
  for (auto &child : children(node))
    apply_recurse_if(*child, predicate, transform);
}
} // namespace pas::ast
