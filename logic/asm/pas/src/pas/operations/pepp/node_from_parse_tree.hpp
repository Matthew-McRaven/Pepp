#pragma once
#include "arg_from_parse_tree.hpp"
#include "node_from_parse_tree.hpp"
#include "pas/ast/generic/attr_comment.hpp"
#include "pas/ast/generic/attr_comment_indent.hpp"
#include "pas/ast/generic/attr_symbol.hpp"
#include "pas/ast/generic/attr_type.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/pepp/attr_addr.hpp"
#include "pas/ast/pepp/attr_instruction.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/parse/pepp/rules_lines.hpp"
#include "symbol/table.hpp"
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>
#include <pas/ast/generic/attr_argument.hpp>
namespace pas::operations::pepp {
using namespace pas::parse::pepp;
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
toAST(const std::vector<pas::parse::pepp::LineType> &lines) {
  auto root = QSharedPointer<pas::ast::Node>::create();
  root->set(ast::generic::SymbolTable{
      .value = QSharedPointer<symbol::Table>::create()});
  QSharedPointer<pas::ast::Node> activeSection;
  auto visitor = FromParseTree<ISA>();
  auto createActive = [&]() {
    activeSection = QSharedPointer<pas::ast::Node>::create();
    activeSection->set(ast::generic::SymbolTable{
        .value = root->get<ast::generic::SymbolTable>().value->addChild()});
    ast::addChild(*root, activeSection);
    visitor.symTab = activeSection->get<ast::generic::SymbolTable>().value;
  };
  createActive();
  for (const auto &line : lines) {
    auto node = line.apply_visitor(visitor);
    activeSection->addChild(node);
  }
  return root;
}

namespace detail {
using ST = QSharedPointer<symbol::Table>;
using namespace pas::parse::pepp;
using namespace pas::ast;

template <typename T>
QList<QSharedPointer<pas::ast::value::Base>>
parse_arg(const T &line, ST symTab, bool preferIdent = false) {
  auto visitor = pas::operations::pepp::ParseToArg();
  visitor.symTab = symTab;
  visitor.preferIdent = preferIdent;
  QList<QSharedPointer<pas::ast::value::Base>> arguments;
  for (const auto &arg : line.args) {
    auto shared = arg.apply_visitor(visitor);
    if (visitor.error)
      throw std::logic_error("Error");
    arguments.push_back(shared);
  }
  return arguments;
}

// Most code (except name) is shared between INPUT/OUTPUT, IMPORT/EXPORT,
// SCALL/USCALL. This fn provides the shared implementation.
QSharedPointer<Node> gen_io_scall_extern(const DirectiveType &line, ST symTab,
                                         QString directive);
QSharedPointer<Node> align(const DirectiveType &line, ST symTab);
QSharedPointer<Node> ascii(const DirectiveType &line, ST symTab);
QSharedPointer<Node> block(const DirectiveType &line, ST symTab);
QSharedPointer<Node> burn(const DirectiveType &line, ST symTab);
QSharedPointer<Node> byte(const DirectiveType &line, ST symTab);
QSharedPointer<Node> end(const DirectiveType &line, ST symTab);
QSharedPointer<Node> equate(const DirectiveType &line, ST symTab);
QSharedPointer<Node> _export(const DirectiveType &line, ST symTab);
QSharedPointer<Node> import(const DirectiveType &line, ST symTab);
QSharedPointer<Node> input(const DirectiveType &line, ST symTab);
QSharedPointer<Node> output(const DirectiveType &line, ST symTab);
QSharedPointer<Node> scall(const DirectiveType &line, ST symTab);
QSharedPointer<Node> section(const DirectiveType &line, ST symTab);
QSharedPointer<Node> uscall(const DirectiveType &line, ST symTab);
QSharedPointer<Node> word(const DirectiveType &line, ST symTab);

} // namespace detail
template <typename ISA>
QSharedPointer<pas::ast::Node>
pas::operations::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::BlankType &line) {
  return QSharedPointer<pas::ast::Node>::create(
      ast::generic::Type{.value = ast::generic::Type::Blank});
}

template <typename ISA>
QSharedPointer<pas::ast::Node>
pas::operations::pepp::FromParseTree<ISA>::operator()(
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
QSharedPointer<pas::ast::Node>
pas::operations::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::UnaryType &line) {
  using pas::ast::generic::Type;
  auto ret =
      QSharedPointer<pas::ast::Node>::create(Type{.value = Type::Instruction});

  if (!line.symbol.empty())
    ret->set(ast::generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  auto instr = ISA::parseMnemonic(QString::fromStdString(line.identifier));
  if (instr == ISA::Mnemonic::INVALID)
    throw std::logic_error("Error");
  ret->set(ast::pepp::Instruction<ISA>{.value = instr});

  if (line.hasComment)
    ret->set(
        ast::generic::Comment{.value = QString::fromStdString(line.comment)});

  return ret;
}

template <typename ISA>
QSharedPointer<pas::ast::Node>
pas::operations::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::NonUnaryType &line) {
  using pas::ast::generic::Type;
  auto ret =
      QSharedPointer<pas::ast::Node>::create(Type{.value = Type::Instruction});

  if (!line.symbol.empty())
    ret->set(ast::generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  auto instr = ISA::parseMnemonic(QString::fromStdString(line.identifier));
  if (instr == ISA::Mnemonic::INVALID)
    throw std::logic_error("Error");
  ret->set(ast::pepp::Instruction<ISA>{.value = instr});

  // Validate that arg is appropriate for instruction.
  auto visitor = pas::operations::pepp::ParseToArg();
  visitor.symTab = symTab;
  auto arg = line.arg.apply_visitor(visitor);
  if (!(arg->isFixedSize() && arg->isNumeric()))
    throw std::logic_error("Error");
  ret->set(ast::generic::Argument{.value = arg});

  // Validate addressing mode is appropriate for instruction.
  if (line.addr.empty() && ISA::requiresAddressingMode(instr))
    throw std::logic_error("Error");
  else if (!line.addr.empty()) {
    auto addr = ISA::parseAddressingMode(QString::fromStdString(line.addr));
    if (ISA::isAType(instr) && !ISA::isValidATypeAddressingMode(addr))
      throw std::logic_error("Error");
    else if (ISA::isAAAType(instr) && !ISA::isValidAAATypeAddressingMode(addr))
      throw std::logic_error("Error");
    else if (ISA::isRAAAType(instr) &&
             !ISA::isValidRAAATypeAddressingMode(addr))
      throw std::logic_error("Error");
    ret->set(ast::pepp::AddressingMode<ISA>{.value = addr});
  }

  if (line.hasComment)
    ret->set(
        ast::generic::Comment{.value = QString::fromStdString(line.comment)});

  return ret;
}
template <typename ISA>
QSharedPointer<pas::ast::Node>
pas::operations::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::DirectiveType &line) {
  using convert_fn = std::function<QSharedPointer<pas::ast::Node>(
      const pas::parse::pepp::DirectiveType &, QSharedPointer<symbol::Table>)>;

  static QMap<QString, convert_fn> converters = {
      {u"ALIGN"_qs, detail::align},     {u"ASCII"_qs, detail::ascii},
      {u"BLOCK"_qs, detail::block},     {u"BURN"_qs, detail::burn},
      {u"BYTE"_qs, detail::byte},       {u"END"_qs, detail::end},
      {u"EQUATE"_qs, detail::equate},   {u"EXPORT"_qs, detail::_export},
      {u"IMPORT"_qs, detail::import},   {u"INPUT"_qs, detail::input},
      {u"OUTPUT"_qs, detail::output},   {u"SCALL"_qs, detail::scall},
      {u"SECTION"_qs, detail::section}, {u"USCALL"_qs, detail::uscall},
      {u"WORD"_qs, detail::word}};

  auto identifier = QString::fromStdString(line.identifier).toUpper();
  if (auto converter = converters.find(identifier);
      converter != converters.end()) {
    return converter.value()(line, symTab);
  }
  throw std::logic_error("No match");
}

template <typename ISA>
QSharedPointer<pas::ast::Node>
pas::operations::pepp::FromParseTree<ISA>::operator()(
    const pas::parse::pepp::MacroType &line) {
  return nullptr;
}

template <typename ISA>
template <typename T>
QSharedPointer<ast::Node> FromParseTree<ISA>::operator()(const T &line) {
  return nullptr;
}
}; // namespace pas::operations::pepp
