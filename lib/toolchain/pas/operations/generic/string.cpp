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
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_comment.hpp"
#include "toolchain/pas/ast/generic/attr_comment_indent.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/generic/attr_macro.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/symbol/entry.hpp"

QString pas::ops::generic::detail::formatBlank(const ast::Node &node, SourceOptions opts) { return ""; }

QString pas::ops::generic::detail::formatComment(const ast::Node &node, SourceOptions opts) {
  QString lpad = "%1;%2";
  QString nopad = ";%1";
  const auto comment = node.get<ast::generic::Comment>().value;
  // TODO: Allow "size" of instruction ident to be variable
  if (node.has<ast::generic::CommentIndent>()) {
    switch (node.get<ast::generic::CommentIndent>().value) {
    case ast::generic::CommentIndent::Level::Left: return nopad.arg(comment);
    case ast::generic::CommentIndent::Level::Instruction: return lpad.arg(" ", 8).arg(comment);
    }
  }
  // Default to left-aligned.
  return nopad.arg(comment);
}

QString pas::ops::generic::detail::format(const QString &symbol, const QString &invoke, const QStringList &args,
                                          const QString &comment, int indentMnemonic, bool spaceAfterComma) {
  using namespace Qt::StringLiterals;
  constexpr int defaultSymWidth = 9;
  int symWidth = qMax(0, defaultSymWidth + indentMnemonic);
  auto joinedArgs = args.join(spaceAfterComma ? ", " : ",");
  auto emptySymPlaceHolder = u" "_s.repeated(symWidth);
  auto symPlaceholder = symbol.isEmpty() ? emptySymPlaceHolder : u"%1"_s.arg(symbol + ":", -symWidth, QChar(' '));
  auto ret = u"%1%2%3%4"_s.arg(symPlaceholder)
                 .arg(invoke, -8, ' ')
                 .arg(joinedArgs, -12, ' ')
                 .arg(comment.isEmpty() ? "" : ";" + comment);
  return ret;
}

QString pas::ops::generic::detail::formatDirectiveOrMacro(const pas::ast::Node &node, const QString &invoke,
                                                          SourceOptions opts) {
  QString symbol = "";
  if (node.has<pas::ast::generic::SymbolDeclaration>()) {
    symbol = node.get<pas::ast::generic::SymbolDeclaration>().value->name;
  }

  QStringList args = {};
  if (node.has<pas::ast::generic::Argument>()) {
    args.push_back(node.get<pas::ast::generic::Argument>().value->string());
  } else if (node.has<pas::ast::generic::ArgumentList>()) {
    for (auto &arg : node.get<pas::ast::generic::ArgumentList>().value) args.push_back(arg->string());
  }

  QString comment = "";
  if (node.has<pas::ast::generic::Comment>()) comment = node.get<pas::ast::generic::Comment>().value;
  if (opts.printErrors) comment += formatErrorsAsComments(node);
  return detail::format(symbol, invoke, args, comment, opts.indentMnemonic, false);
}

QString pas::ops::generic::detail::formatDirective(const ast::Node &node, SourceOptions opts) {
  using namespace Qt::StringLiterals;
  if (!node.has<ast::generic::Directive>()) {
    static const char *const e = "Directive missing directive element";
    qCritical(e);
    throw std::logic_error(e);
  }
  return formatDirectiveOrMacro(node, u".%1"_s.arg(node.get<ast::generic::Directive>().value), opts);
}

QString pas::ops::generic::detail::formatMacro(const ast::Node &node, SourceOptions opts) {
  using namespace Qt::StringLiterals;
  if (!node.has<ast::generic::Macro>()) {
    static const char *const e = "Macro missing directive macro";
    qCritical(e);
    throw std::logic_error(e);
  }
  auto newOpts = opts;
  newOpts.indentMnemonic = 1;
  return formatDirectiveOrMacro(node, u"@%1"_s.arg(node.get<ast::generic::Macro>().value), newOpts);
}

QString pas::ops::generic::detail::formatErrorsAsComments(const ast::Node &node) {
  using namespace Qt::StringLiterals;
  QString ret = "";
  if (node.has<pas::ast::generic::Error>())
    for (auto &error : node.get<pas::ast::generic::Error>().value) ret += u";ERROR: %1"_s.arg(error.message);

  return ret;
}
