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

#include "./string.hpp"
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/generic/attr_comment.hpp"
#include "pas/ast/generic/attr_comment_indent.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/generic/attr_macro.hpp"
#include "pas/ast/generic/attr_symbol.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/value/base.hpp"
#include "symbol/entry.hpp"

QString pas::ops::generic::detail::formatBlank(const ast::Node &node,
                                               SourceOptions opts) {
  return u""_qs;
}

QString pas::ops::generic::detail::formatComment(const ast::Node &node,
                                                 SourceOptions opts) {
  QString lpad = "%1;%2";

  // TODO: Allow "size" of instruction ident to be variable
  if (node.has<ast::generic::CommentIndent>()) {
    switch (node.get<ast::generic::CommentIndent>().value) {
    case ast::generic::CommentIndent::Level::Left:
      lpad = lpad.arg(" ", 0);
      break;
    case ast::generic::CommentIndent::Level::Instruction:
      lpad = lpad.arg(" ", 8);
      break;
    }
  } else {
    // Default to left-aligned.
    lpad = lpad.arg(" ", 0);
  }
  return lpad.arg(node.get<ast::generic::Comment>().value);
}

QString pas::ops::generic::detail::format(QString symbol, QString invoke,
                                          QStringList args, QString comment) {
  auto joinedArgs = args.join(", ");
  auto symPlaceholder = symbol.isEmpty()
                            ? u"        "_qs
                            : u"%1"_qs.arg(symbol + ":", -9, QChar(' '));
  auto ret = u"%1%2%3%4"_qs.arg(symPlaceholder)
                 .arg(invoke, -8, ' ')
                 .arg(joinedArgs, -8, ' ')
                 .arg(comment.isEmpty() ? "" : ";" + comment);
  return ret;
}

QString pas::ops::generic::detail::formatDirectiveOrMacro(
    const pas::ast::Node &node, QString invoke, SourceOptions opts) {
  QString symbol = "";
  if (node.has<pas::ast::generic::SymbolDeclaration>()) {
    symbol = node.get<pas::ast::generic::SymbolDeclaration>().value->name;
  }

  QStringList args = {};
  if (node.has<pas::ast::generic::Argument>()) {
    args.push_back(node.get<pas::ast::generic::Argument>().value->string());
  } else if (node.has<pas::ast::generic::ArgumentList>()) {
    for (auto &arg : node.get<pas::ast::generic::ArgumentList>().value)
      args.push_back(arg->string());
  }

  QString comment = "";
  if (node.has<pas::ast::generic::Comment>())
    comment = node.get<pas::ast::generic::Comment>().value;
  if (opts.printErrors)
    comment += formatErrorsAsComments(node);
  return detail::format(symbol, invoke, args, comment);
}

QString pas::ops::generic::detail::formatDirective(const ast::Node &node,
                                                   SourceOptions opts) {
  if (!node.has<ast::generic::Directive>())
    throw std::logic_error("Directive missing directive element");
  return formatDirectiveOrMacro(
      node, u".%1"_qs.arg(node.get<ast::generic::Directive>().value), opts);
}

QString pas::ops::generic::detail::formatMacro(const ast::Node &node,
                                               SourceOptions opts) {
  if (!node.has<ast::generic::Macro>())
    throw std::logic_error("Macro missing directive macro");
  return formatDirectiveOrMacro(
      node, u"@%1"_qs.arg(node.get<ast::generic::Macro>().value), opts);
}

QString
pas::ops::generic::detail::formatErrorsAsComments(const ast::Node &node) {
  QString ret = "";
  if (node.has<pas::ast::generic::Error>())
    for (auto &error : node.get<pas::ast::generic::Error>().value)
      ret += u";ERROR: %1"_qs.arg(error.message);

  return ret;
}
