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
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/op.hpp"
#include "toolchain/pas/errors.hpp"
#include "toolchain/pas/operations/generic/find.hpp"
#include "toolchain/pas/operations/generic/is.hpp"
#include "toolchain/pas/operations/pepp/find.hpp"
#include "toolchain/pas/operations/pepp/size.hpp"

#include <toolchain/pas/ast/generic/attr_address.hpp>

namespace pas::ops::pepp {
template <typename ISA> struct ValidateDirectives : public pas::ops::MutatingOp<void> {
  bool valid = true;
  void operator()(ast::Node &node) {
    auto localValid = ISA::isLegalDirective(node.get<pas::ast::generic::Directive>().value);
    valid &= localValid;
    if (!localValid) {
      auto message = pas::errors::pepp::illegalDirective.arg("." + node.get<pas::ast::generic::Directive>().value);
      ast::addError(node, {.severity = ast::generic::Message::Severity::Fatal, .message = message});
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
  bool ignoreUndefinedSymbols = false;
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

template <typename ISA> struct AnnotateRetOps : public pas::ops::MutatingOp<void> {
  AnnotateRetOps(QSet<quint16> &addresses) : addresses(addresses) {}
  void operator()(ast::Node &node) {
    if (node.has<ast::pepp::Instruction<ISA>>()) {
      auto mn = node.get<ast::pepp::Instruction<ISA>>().value;
      if (mn != ISA::Mnemonic::RET) return;
      else if (!node.has<ast::generic::Comment>()) return;
      if (auto com = node.get<ast::generic::Comment>().value; !com.toLower().contains("@call")) return;
      if (!node.has<ast::generic::Address>()) return;
      addresses.insert(node.get<ast::generic::Address>().value.start);
    }
  }
  QSet<quint16> &addresses;
};

template <typename ISA> void annotateRetOps(QSet<quint16> &ret, ast::Node &node) {
  AnnotateRetOps<ISA> visit(ret);
  ast::apply_recurse(node, visit);
}

template <typename ISA> bool checkWholeProgramSanity(ast::Node &node, Features features) {
  if (implicitSize<ISA>(node) > 0x10000) {
    // Add error to first "real" node
    auto target = ops::generic::findFirst(node, pas::ops::pepp::findNonStructural);
    // No need to error check. If size is non-zero, there must exist some node
    // that is non-structural.
    ast::addError(target,
                  {.severity = pas::ast::generic::Message::Severity::Fatal, .message = pas::errors::pepp::objTooBig});
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
  else if (!features.ignoreUndefinedSymbols && errorOnUndefinedSymbolicArgument(node))
    // Visitor adds its own errors, just signal error
    return false;
  else if (errorOnMultipleSymbolDefiniton(node))
    // Visitor adds its own errors, just signal error
    return false;

  return true;
}
} // namespace pas::ops::pepp
