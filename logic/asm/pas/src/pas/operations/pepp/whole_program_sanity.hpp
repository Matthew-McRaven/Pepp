#pragma once
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/op.hpp"
#include "pas/errors.hpp"
#include "pas/operations/generic/find.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/find.hpp"

namespace pas::ops::pepp {
template <typename ISA>
struct ValidateDirectives : public pas::ops::MutatingOp<void> {
  bool valid = true;
  void operator()(ast::Node &node) {
    auto localValid =
        ISA::isLegalDirective(node.get<pas::ast::generic::Directive>().value);
    valid &= localValid;
    if (!localValid) {
      auto message = pas::errors::pepp::illegalDirective.arg(
          "." + node.get<pas::ast::generic::Directive>().value);
      ast::addError(node, {.severity = ast::generic::Message::Severity::Fatal,
                           .message = message});
    }
  }
};

template <typename ISA> bool validateDirectives(ast::Node &node) {
  ValidateDirectives<ISA> dirs;
  ops::generic::isDirective is;
  ast::apply_recurse_if(node, is, dirs);
  return dirs.valid;
}

struct IsOSFeature : public pas::ops::ConstOp<bool> {
  bool operator()(const ast::Node &node);
};

bool hasOSFeatures(ast::Node &node);

struct ErrorOnOSFeatures : public pas::ops::MutatingOp<void> {
  void operator()(ast::Node &node);
};

void errorOnOSFeatures(ast::Node &node);

struct Features {
  bool allowOSFeatures = false;
};

struct ErrorOnUndefinedSymbolicArgument : public pas::ops::MutatingOp<void> {
  bool hadError = false;
  void operator()(ast::Node &node);
};

bool errorOnUndefinedSymbolicArgument(ast::Node &node);

struct ErrorOnMultipleSymbolDefiniton : public pas::ops::MutatingOp<void> {
  bool hadError = false;
  void operator()(ast::Node &node);
};

bool errorOnMultipleSymbolDefiniton(ast::Node &node);

template <typename ISA>
bool checkWholeProgramSanity(ast::Node &node, Features features) {
  if (implicitSize<ISA>(node) > 0x10000) {
    // Add error to first "real" node
    auto target =
        ops::generic::findFirst(node, pas::ops::pepp::findNonStructural);
    // No need to error check. If size is non-zero, there must exist some node
    // that is non-structural.
    ast::addError(target,
                  {.severity = pas::ast::generic::Message::Severity::Fatal,
                   .message = pas::errors::pepp::objTooBig});
    return false;
  } else if (!validateDirectives<ISA>(node))
    // Visitor adds its own errors, just signal error
    return false;
  else if (hasOSFeatures(node) && !features.allowOSFeatures) {
    // Add error to all nodes with OS features
    errorOnOSFeatures(node);
    return false;
  } /*else if (ops::generic::findFirst(node, pas::ops::pepp::findUnhiddenEnd) ==
             nullptr) {
    // Add error to first "real" node if present
    auto target =
        ops::generic::findFirst(node, pas::ops::pepp::findNonStructural);
    // BUG: will throw when program is empty.
    if (target == nullptr)
      throw std::logic_error(
          "Unhandled nullptr in pepp::whole_program_sanity.");
    ast::addError(target,
                  {.severity = pas::ast::generic::Message::Severity::Fatal,
                   .message = pas::errors::pepp::missingEnd});
    return false;
  }*/
  else if (errorOnUndefinedSymbolicArgument(node))
    // Visitor adds its own errors, just signal error
    return false;
  else if (errorOnMultipleSymbolDefiniton(node))
    // Visitor adds its own errors, just signal error
    return false;

  return true;
}
} // namespace pas::ops::pepp
