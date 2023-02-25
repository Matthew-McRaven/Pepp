#pragma once
#include "pas/ast/op.hpp"
#include <QtCore>
namespace pas::ast {
class Node;
}
namespace pas::ops::generic {
struct isBlank : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};
struct isComment : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isAlign : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isString : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"STRING"_qs};
  bool operator()(const ast::Node &node);
};

struct isSkip : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"SKIP"_qs};
  bool allowFill = false;
  bool operator()(const ast::Node &node);
};

struct isByte1 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"BYTE1"_qs};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct isByte2 : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"BYTE2"_qs};
  bool allowMultiple = false;
  bool operator()(const ast::Node &node);
};
struct isSet : public pas::ops::ConstOp<bool> {
  QList<QString> directiveAliases = {u"SET"_qs};
  bool operator()(const ast::Node &node);
};
} // namespace pas::ops::generic
