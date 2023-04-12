#pragma once
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/ast/pepp/attr_addr.hpp"
#include "pas/ast/pepp/attr_instruction.hpp"

namespace pas::ops::pepp {

template <typename ISA> struct isUnary : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename ISA> struct isNonUnary : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename ISA> struct isUType : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename ISA> struct isRType : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename ISA> struct isAType : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename ISA> struct isAAAType : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

template <typename ISA> struct isRAAAType : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isBurn : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isEnd : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isExport : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isImport : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isInput : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isOrg : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isOutput : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isSCall : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isSection : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

struct isUSCall : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

} // namespace pas::ops::pepp

template <typename ISA>
bool pas::ops::pepp::isUnary<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  else if (node.get<ast::generic::Type>().value !=
           ast::generic::Type::Instruction)
    return false;
  auto instr = node.get<ast::pepp::Instruction<ISA>>().value;
  return ISA::isUType(instr) || ISA::isRType(instr);
}

template <typename ISA>
bool pas::ops::pepp::isNonUnary<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  else if (node.get<ast::generic::Type>().value !=
           ast::generic::Type::Instruction)
    return false;
  auto instr = node.get<ast::pepp::Instruction<ISA>>().value;
  return ISA::isAType(instr) || ISA::isAAAType(instr) || ISA::isRAAAType(instr);
}

template <typename ISA>
bool pas::ops::pepp::isUType<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  return ISA::isUType(node.get<ast::pepp::Instruction<ISA>>().value) &&
         node.get<ast::generic::Type>().value ==
             ast::generic::Type::Instruction;
}

template <typename ISA>
bool pas::ops::pepp::isRType<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  return ISA::isRType(node.get<ast::pepp::Instruction<ISA>>().value) &&
         node.get<ast::generic::Type>().value ==
             ast::generic::Type::Instruction;
}

template <typename ISA>
bool pas::ops::pepp::isAType<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  else if (!node.has<ast::generic::Argument>())
    return false;
  return ISA::isAType(node.get<ast::pepp::Instruction<ISA>>().value) &&
         node.get<ast::generic::Type>().value ==
             ast::generic::Type::Instruction;
}

template <typename ISA>
bool pas::ops::pepp::isAAAType<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  else if (!node.has<ast::generic::Argument>())
    return false;
  else if (!node.has<ast::pepp::AddressingMode<ISA>>())
    return false;
  return ISA::isAAAType(node.get<ast::pepp::Instruction<ISA>>().value) &&
         node.get<ast::generic::Type>().value ==
             ast::generic::Type::Instruction;
}

template <typename ISA>
bool pas::ops::pepp::isRAAAType<ISA>::operator()(const ast::Node &node) {
  if (!node.has<ast::pepp::Instruction<ISA>>())
    return false;
  else if (!node.has<ast::generic::Argument>())
    return false;
  else if (!node.has<ast::pepp::AddressingMode<ISA>>())
    return false;
  return ISA::isRAAAType(node.get<ast::pepp::Instruction<ISA>>().value) &&
         node.get<ast::generic::Type>().value ==
             ast::generic::Type::Instruction;
}
