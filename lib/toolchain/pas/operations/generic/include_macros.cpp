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

#include "include_macros.hpp"
#include "is.hpp"
#include "toolchain/macro/declaration.hpp"
#include "toolchain/macro/registry.hpp"
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_children.hpp"
#include "toolchain/pas/ast/generic/attr_comment_indent.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/generic/attr_macro.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/ast/value/decimal.hpp"
#include "toolchain/pas/driver/common.hpp"
#include "toolchain/pas/errors.hpp"
#include "toolchain/pas/operations/generic/errors.hpp"
#include "toolchain/pas/operations/generic/string.hpp"

#include <toolchain/pas/ast/generic/attr_comment.hpp>

void appendError(pas::ast::Node &node, QString message) {
  using namespace pas::ast::generic;
  Error err;
  if (node.has<Error>()) err = node.get<Error>();
  err.value.push_back(Message{.severity = Message::Severity::Fatal, .message = message});
  node.set(err);
}

bool pas::ops::generic::IncludeMacros::operator()(ast::Node &node) {
  using namespace Qt::StringLiterals;
  // Node should be a macro
  if (!node.has<ast::generic::Macro>()) {
    appendError(node, errors::pepp::expectedAMacro);
    return false;
  }
  auto macroName = node.get<ast::generic::Macro>().value;
  QStringList args = {};
  if (node.has<ast::generic::ArgumentList>()) {
    for (auto &arg : node.get<ast::generic::ArgumentList>().value) args.push_back(arg->string());
  } else if (node.has<ast::generic::Argument>()) args.push_back(node.get<ast::generic::Argument>().value->string());

  auto macroInvoke = MacroInvocation{.macroName = macroName, .args = args};
  if (!pushMacroInvocation(macroInvoke)) {
    appendError(node, errors::pepp::macroLoop);
    return false;
  }
  auto maybeMacro = registry->findMacro(macroName);
  if (maybeMacro == nullptr) {
    appendError(node, errors::pepp::noSuchMacro.arg(macroName));
    return false;
  }
  auto macroContents = maybeMacro->contents();
  if (args.size() != macroContents->argCount()) {
    appendError(node, errors::pepp::macroWrongArity.arg(macroName).arg(macroContents->argCount()).arg(args.size()));
    return false;
  }
  // Perform macro arg substitution on body.
  auto macroText = macroContents->body();
  for (int it = 0; it < args.size(); it++) macroText = macroText.replace(u"$"_s + QString::number(it + 1), args[it]);

  // Function handles parenting macroText's nodes as node's children.
  // Parent/child relationships also established.
  auto converted = convertFn(macroText, node.sharedFromThis());

  if (converted.hadError) {
    for (auto &error : converted.errors) appendError(node, error);
    return false;
  }

  popMacroInvocation(macroInvoke);

  // Add start / end comment and bind symbol to line (or equate) if present
  addExtraChildren(node);

  return true;
}

bool pas::ops::generic::IncludeMacros::pushMacroInvocation(MacroInvocation invoke) {
  if (_chain.contains(invoke)) return false;
  _chain.insert(invoke);
  return true;
}

void pas::ops::generic::IncludeMacros::popMacroInvocation(MacroInvocation invoke) { _chain.remove(invoke); }

void pas::ops::generic::IncludeMacros::addExtraChildren(ast::Node &node) {
  using namespace Qt::StringLiterals;
  auto children = ast::children(node);

  static const auto commentType = ast::generic::Type{.value = ast::generic::Type::Comment};
  static const auto directiveType = ast::generic::Type{.value = ast::generic::Type::Directive};

  // Must generate start comment before removing symbol declaration.
  auto start = QSharedPointer<ast::Node>::create(commentType);
  start->set(ast::generic::IsMacroComment{});
  start->set(ast::generic::CommentIndent{.value = ast::generic::CommentIndent::Level::Left});
  // Align the macro comment as if it were an instruction.
  // Need a -2 indent to accomodate the ;@ characters.
  auto formattedMacro = detail::formatMacro(node, {.indentMnemonic = -2});
  start->set(ast::generic::Comment{.value = formattedMacro});

  // If the macro declares a symbol, find the first addressable line and attempt to move our symbol to that line.
  // Macros may start with newlines or comment lines, which we must skip over for moving our symbok
  // If the first addressable line already has a symbol, we will resort to a .BLOCK 0
  if (node.has<ast::generic::SymbolDeclaration>()) {
    bool placedSymbol = false;
    for (const auto &child : std::as_const(children)) {
      if (child->get<ast::generic::Type>().value == ast::generic::Type::Instruction || isDirectiveAddressed(*child)) {
        if (child->has<ast::generic::SymbolDeclaration>()) break; // Line already has symbol, opt for .BLOCK 0 instead.
        else { // No existing symbol, can move macro's symbol to here.
          child->set(node.get<ast::generic::SymbolDeclaration>());
          placedSymbol = true;
          break;
        }
      }
    }
    // First addressable line already had a symbol, insert .BLOCK 0 at the top of the macro expansion.
    if (!placedSymbol) {
      auto sym = QSharedPointer<ast::Node>::create(directiveType);
      sym->set(ast::generic::Directive{.value = "BLOCK"});
      sym->set(node.get<ast::generic::SymbolDeclaration>());
      auto zero = QSharedPointer<ast::value::UnsignedDecimal>::create(0, 2);
      sym->set(ast::generic::Argument{.value = zero});
      children.push_front(sym);
    }
  }

  children.push_front(start);

  auto end = QSharedPointer<ast::Node>::create(commentType);
  end->set(ast::generic::IsMacroComment{});
  end->set(ast::generic::CommentIndent{.value = ast::generic::CommentIndent::Level::Left});
  // TODO: enable translations.
  auto spacing = u" "_s.repeated(indents::defaultSymWidth - 1);
  // Align the macro comment as if it were an instruction.
  // Need a -1 indent to accomodate the ; character.
  end->set(
      ast::generic::Comment{.value = u"%1End @%2"_s.arg(spacing, node.get<ast::generic::Macro>().value.toUpper())});
  children.push_back(end);

  node.set<ast::generic::Children>(ast::generic::Children{.value = children});
}
