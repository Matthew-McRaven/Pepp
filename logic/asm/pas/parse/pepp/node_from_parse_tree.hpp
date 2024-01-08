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
#include "arg_from_parse_tree.hpp"
#include "node_from_parse_tree.hpp"
#include "asm/pas/ast/generic/attr_comment.hpp"
#include "asm/pas/ast/generic/attr_comment_indent.hpp"
#include "asm/pas/ast/generic/attr_error.hpp"
#include "asm/pas/ast/generic/attr_hide.hpp"
#include "asm/pas/ast/generic/attr_location.hpp"
#include "asm/pas/ast/generic/attr_macro.hpp"
#include "asm/pas/ast/generic/attr_symbol.hpp"
#include "asm/pas/ast/generic/attr_type.hpp"
#include "asm/pas/ast/node.hpp"
#include "asm/pas/ast/pepp/attr_addr.hpp"
#include "asm/pas/ast/pepp/attr_instruction.hpp"
#include "asm/pas/ast/value/base.hpp"
#include "asm/pas/ast/value/hexadecimal.hpp"
#include "asm/pas/ast/value/string.hpp"
#include "asm/pas/errors.hpp"
#include "asm/pas/operations/pepp/is.hpp"
#include "asm/pas/parse/pepp/types_lines.hpp"
#include "asm/symbol/table.hpp"
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>
#include <asm/pas/ast/generic/attr_argument.hpp>
#include <asm/pas/ast/generic/attr_directive.hpp>
#include <asm/pas/ast/generic/attr_sec.hpp>
#include "asm/pas/pas_globals.hpp"

namespace pas::parse::pepp {
template <typename ISA>
struct FromParseTree
    : public boost::static_visitor<QSharedPointer<pas::ast::Node>> {
  QSharedPointer<symbol::Table> symTab;
  QSharedPointer<pas::ast::Node> operator()(const BlankType &line);
  QSharedPointer<pas::ast::Node> operator()(const CommentType &line);
  QSharedPointer<pas::ast::Node> operator()(const UnaryType &line);
  QSharedPointer<pas::ast::Node> operator()(const NonUnaryType &line);
  QSharedPointer<pas::ast::Node> operator()(const DirectiveType &line);
  QSharedPointer<pas::ast::Node> operator()(const MacroType &line);
  template <typename T>
  QSharedPointer<pas::ast::Node> operator()(const T &line);
};

template <typename ISA>
QSharedPointer<pas::ast::Node>
toAST(const std::vector<pas::parse::pepp::LineType> &lines,
      QSharedPointer<pas::ast::Node> parent = nullptr, bool hideEnd = false) {
  if (parent.isNull()) {
    static const auto structuralType =
        ast::generic::Type{.value = ast::generic::Type::Structural};
    parent = QSharedPointer<pas::ast::Node>::create(structuralType);
    parent->set(ast::generic::SymbolTable{
        .value = QSharedPointer<symbol::Table>::create(2)});
  }
  auto visitor = FromParseTree<ISA>();
  visitor.symTab = parent->get<ast::generic::SymbolTable>().value;
  qsizetype loc = 0;
  for (const auto &line : lines) {
    QSharedPointer<ast::Node> node = line.apply_visitor(visitor);
    node->set(
        ast::generic::SourceLocation{.value = {.line = loc++, .valid = true}});

    // Only apply hideEnd if the node is an .END directive
    if (ast::type(*node).value == ast::generic::Type::Directive &&
        node->get<ast::generic::Directive>().value == "END") {
      if (node->has<ast::generic::Hide>()) {
        auto hide = node->get<ast::generic::Hide>().value;
        hide.source = hideEnd;
        hide.listing = hideEnd;
        node->set(ast::generic::Hide{.value = hide});
      } else
        node->set(ast::generic::Hide{
            .value = {.source = hideEnd, .listing = hideEnd}});
    }

    // Grouping into sections is now handled in ops::treeify.
    ast::addChild(*parent, node);
  }
  return parent;
}

namespace detail {
using ST = QSharedPointer<symbol::Table>;
using namespace pas::parse::pepp;
using namespace pas::ast;

template <typename T>
QList<QSharedPointer<pas::ast::value::Base>>
parse_arg(const T &line, ST symTab, bool preferIdent = false,
          std::optional<quint8> sizeOverride = std::nullopt) {
  auto visitor = pas::parse::pepp::ParseToArg();
  visitor.symTab = symTab;
  visitor.preferIdent = preferIdent;
  visitor.sizeOverride = sizeOverride;
  QList<QSharedPointer<pas::ast::value::Base>> arguments;
  for (const auto &arg : line.args) {
    auto shared = arg.apply_visitor(visitor);
    arguments.push_back(shared);
  }
  return arguments;
}

// Most code (except name) is shared between INPUT/OUTPUT, IMPORT/EXPORT,
// SCALL/USCALL. This fn provides the shared implementation.
QSharedPointer<Node> PAS_EXPORT gen_io_scall_extern(const DirectiveType &line, ST symTab,
                                         QString directive);
QSharedPointer<Node> PAS_EXPORT align(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT ascii(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT block(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT burn(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT byte(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT end(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT equate(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT _export(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT import(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT input(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT output(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT org(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT scall(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT section(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT uscall(const DirectiveType &line, ST symTab);
QSharedPointer<Node> PAS_EXPORT word(const DirectiveType &line, ST symTab);
template <typename ISA> void checkArgumentSizes(QSharedPointer<Node>);

} // namespace detail
template <typename ISA>
QSharedPointer<pas::ast::Node> pas::parse::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::BlankType &line) {
  return QSharedPointer<pas::ast::Node>::create(
      ast::generic::Type{.value = ast::generic::Type::Blank});
}

template <typename ISA>
QSharedPointer<pas::ast::Node> pas::parse::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::CommentType &line) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      ast::generic::Type{.value = ast::generic::Type::Comment});
  ret->set(
      ast::generic::Comment{.value = QString::fromStdString(line.comment)});
  ret->set(ast::generic::CommentIndent{
      .value = ast::generic::CommentIndent::Level::Left});
  return ret;
}

template <typename ISA>
QSharedPointer<pas::ast::Node> pas::parse::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::UnaryType &line) {
  using pas::ast::generic::Type;
  auto ret =
      QSharedPointer<pas::ast::Node>::create(Type{.value = Type::Instruction});

  if (!line.symbol.empty())
    ret->set(ast::generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  auto instr = ISA::parseMnemonic(QString::fromStdString(line.identifier));

  // Triggered by something of the form symbol: instruction ;comment, where
  // instruction is not a valid mnemonic.
  if (instr == ISA::Mnemonic::INVALID)
    return addError(ret,
                    {.severity = pas::ast::generic::Message::Severity::Fatal,
                     .message = pas::errors::pepp::invalidMnemonic});
  ret->set(ast::pepp::Instruction<ISA>{.value = instr});

  if (line.hasComment)
    ret->set(
        ast::generic::Comment{.value = QString::fromStdString(line.comment)});

  return ret;
}

template <typename ISA>
QSharedPointer<pas::ast::Node> pas::parse::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::NonUnaryType &line) {
  using pas::ast::generic::Type;
  auto ret =
      QSharedPointer<pas::ast::Node>::create(Type{.value = Type::Instruction});

  if (!line.symbol.empty())
    ret->set(ast::generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  auto instr = ISA::parseMnemonic(QString::fromStdString(line.identifier));
  // Triggered by something of the form symbol: instruction arg,addr ;comment,
  // where instruction is not a valid mnemonic.
  if (instr == ISA::Mnemonic::INVALID)
    return addError(ret,
                    {.severity = pas::ast::generic::Message::Severity::Fatal,
                     .message = pas::errors::pepp::invalidMnemonic});
  ret->set(ast::pepp::Instruction<ISA>{.value = instr});

  // Validate that arg is appropriate for instruction.
  auto visitor = pas::parse::pepp::ParseToArg();
  visitor.symTab = symTab;
  auto arg = line.arg.apply_visitor(visitor);
  // Triggered when the argument is a string of length 3.
  if (!(arg->isFixedSize() && arg->isNumeric()))
    return addError(ret,
                    {.severity = pas::ast::generic::Message::Severity::Fatal,
                     .message = pas::errors::pepp::expectedNumeric});
  ret->set(ast::generic::Argument{.value = arg});
  detail::checkArgumentSizes<ISA>(ret);

  // Validate addressing mode is appropriate for instruction.
  // Triggered when a non-branch instruction is missing an addressing mode.
  if (line.addr.empty() && ISA::requiresAddressingMode(instr))
    return addError(ret,
                    {.severity = pas::ast::generic::Message::Severity::Fatal,
                     .message = pas::errors::pepp::requiredAddrMode});
  else if (line.addr.empty()) {
    // TODO: add code to ISA to allow different default addressing modes on a
    // per-instruction basis.
    ret->set(ast::pepp::AddressingMode<ISA>{
        .value = ISA::defaultAddressingMode(instr)});
  } else if (!line.addr.empty()) {
    auto addr = ISA::parseAddressingMode(QString::fromStdString(line.addr));
    // Triggered when an instruction is not in the valid addressing mode set,
    // like "p".
    if (addr == ISA::AddressingMode::INVALID)
      return detail::addError(
          ret, {.severity = pas::ast::generic::Message::Severity::Fatal,
                .message = pas::errors::pepp::invalidAddrMode});
    // Triggered when an addressing mode doesn't work with an instruction, like
    // sfx with br, and i with stwa.
    else if ((ISA::isAType(instr) &&
              !ISA::isValidATypeAddressingMode(instr, addr)) ||
             (ISA::isAAAType(instr) &&
              !ISA::isValidAAATypeAddressingMode(instr, addr)) ||
             (ISA::isRAAAType(instr) &&
              !ISA::isValidRAAATypeAddressingMode(instr, addr)))
      return addError(ret,
                      {.severity = pas::ast::generic::Message::Severity::Fatal,
                       .message = pas::errors::pepp::illegalAddrMode});

    else
      ret->set(ast::pepp::AddressingMode<ISA>{.value = addr});
  }

  if (line.hasComment)
    ret->set(
        ast::generic::Comment{.value = QString::fromStdString(line.comment)});

  return ret;
}
template <typename ISA>
QSharedPointer<pas::ast::Node> pas::parse::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::DirectiveType &line) {
  using convert_fn = std::function<QSharedPointer<pas::ast::Node>(
      const pas::parse::pepp::DirectiveType &, QSharedPointer<symbol::Table>)>;

  static QMap<QString, convert_fn> converters = {
      {u"ALIGN"_qs, detail::align},   {u"ASCII"_qs, detail::ascii},
      {u"BLOCK"_qs, detail::block},   {u"BURN"_qs, detail::burn},
      {u"BYTE"_qs, detail::byte},     {u"END"_qs, detail::end},
      {u"EQUATE"_qs, detail::equate}, {u"EXPORT"_qs, detail::_export},
      {u"IMPORT"_qs, detail::import}, {u"INPUT"_qs, detail::input},
      {u"OUTPUT"_qs, detail::output}, {u"ORG"_qs, detail::org},
      {u"SCALL"_qs, detail::scall},   {u"SECTION"_qs, detail::section},
      {u"USCALL"_qs, detail::uscall}, {u"WORD"_qs, detail::word}};

  auto identifier = QString::fromStdString(line.identifier).toUpper();
  if (auto converter = converters.find(identifier);
      converter != converters.end()) {
    auto ret = converter.value()(line, symTab);
    // Triggers errors when an argument is more than 1 byte (for .BYTE) or more
    // than 2 bytes (except for ASCII and SECTION).
    detail::checkArgumentSizes<ISA>(ret);
    return ret;
  } else {
    auto ret = QSharedPointer<pas::ast::Node>::create(
        ast::generic::Type{.value = ast::generic::Type::Directive});
    ret->set(ast::generic::Directive{
        .value = QString::fromStdString(line.identifier)});
    // Triggered when the dot command is not in the above map.
    return detail::addError(
        ret, {.severity = pas::ast::generic::Message::Severity::Fatal,
              .message = pas::errors::pepp::invalidDirective});
  }
}

template <typename ISA>
QSharedPointer<pas::ast::Node> pas::parse::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::MacroType &line) {
  using Type = pas::ast::generic::Type;
  // Must share root symbol table until we have ability to limit symbol
  // visibility.
  auto ret =
      QSharedPointer<pas::ast::Node>::create(Type{.value = Type::MacroInvoke});
  ret->set(ast::generic::SymbolTable{.value = symTab});
  if (!line.symbol.empty())
    ret->set(ast::generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  ret->set(
      ast::generic::Macro{.value = QString::fromStdString(line.identifier)});
  ret->set(ast::generic::ArgumentList{
      .value = detail::parse_arg(line, symTab, true)});

  if (line.hasComment)
    ret->set(
        ast::generic::Comment{.value = QString::fromStdString(line.comment)});

  return ret;
}

template <typename ISA>
template <typename T>
QSharedPointer<ast::Node> FromParseTree<ISA>::operator()(const T &line) {
  throw std::logic_error("Unimplemented");
}

QString PAS_EXPORT errorFromByteString(QSharedPointer<ast::value::Base> arg);

QString PAS_EXPORT errorFromWordString(QSharedPointer<ast::value::Base> arg);

template <typename ISA>
void detail::checkArgumentSizes(QSharedPointer<ast::Node> node) {
  using S = pas::ast::generic::Message::Severity;
  namespace EP = pas::errors::pepp;
  auto exemptedFromLength = QSet<QString>{u"ASCII"_qs, u"SECTION"_qs};
  if (node->has<ast::generic::Directive>() &&
      node->has<ast::generic::Argument>()) {
    auto directive = node->get<ast::generic::Directive>().value;
    auto arg = node->get<ast::generic::Argument>().value;
    if (directive.toUpper() == u"BYTE"_qs && arg->requiredBytes() > 1)
      addError(node,
               {.severity = S::Fatal, .message = errorFromByteString(arg)});
    else if (!exemptedFromLength.contains(directive.toUpper()) &&
             arg->requiredBytes() > 2)
      addError(node,
               {.severity = S::Fatal, .message = errorFromWordString(arg)});

  } else if (node->has<ast::pepp::Instruction<ISA>>() &&
             node->has<ast::generic::Argument>()) {
    auto mnemonic = node->get<ast::pepp::Instruction<ISA>>().value;
    auto arg = node->get<ast::generic::Argument>().value;
    // TODO: Handle "byte" argument for LDWr x,i
    if (arg->requiredBytes() > 2)
      addError(node,
               {.severity = S::Fatal, .message = errorFromWordString(arg)});
  }
}
}; // namespace pas::parse::pepp
