#include "./node_from_parse_tree.hpp"
#include "pas/ast/value/symbolic.hpp"
#include <pas/ast/generic/attr_argument.hpp>
#include <pas/ast/generic/attr_comment.hpp>
#include <pas/ast/generic/attr_directive.hpp>
#include <pas/ast/generic/attr_symbol.hpp>
#include <pas/ast/value/hexadecimal.hpp>

using ST = QSharedPointer<symbol::Table>;
using namespace pas::parse::pepp;
using namespace pas::ast;

QSharedPointer<Node> pas::operations::pepp::detail::gen_io_scall_extern(
    const DirectiveType &line, ST symTab, QString directive) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (auto as_sym = dynamic_cast<value::Symbolic *>(arg.data());
      as_sym == nullptr)
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    throw std::logic_error("Can't define symbol");
  ret->set(generic::Directive{.value = directive});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

bool isPow2(QSharedPointer<pas::ast::value::Base> arg) {
  quint64 val = 0;
  arg->value(reinterpret_cast<quint8 *>(&val), 8, pas::bits::hostOrder());
  auto val_log2 = log2(val);
  return ceil(val_log2) == floor(val_log2);
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::align(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric() && isPow2(arg)))
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"ALIGN"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::ascii(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (!arg->isText())
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"ASCII"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::block(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"BLOCK"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::burn(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (auto asHex = dynamic_cast<pas::ast::value::Hexadecimal *>(arg.data());
      asHex == nullptr)
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"BURN"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::byte(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"BYTE"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::end(const DirectiveType &line, ST symTab) {
  if (line.args.size() != 0)
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    throw std::logic_error("Error");
  ret->set(generic::Directive{.value = u"END"_qs});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::operations::pepp::detail::equate(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (line.symbol.empty())
    throw std::logic_error("error");
  ret->set(generic::SymbolDeclaration{
      .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"EQUATE"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<Node>
pas::operations::pepp::detail::_export(const DirectiveType &line, ST symTab) {
  return detail::gen_io_scall_extern(line, symTab, "EXPORT");
}

QSharedPointer<Node>
pas::operations::pepp::detail::import(const DirectiveType &line, ST symTab) {
  return detail::gen_io_scall_extern(line, symTab, "IMPORT");
}

QSharedPointer<Node>
pas::operations::pepp::detail::input(const DirectiveType &line, ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "INPUT");
}

QSharedPointer<Node>
pas::operations::pepp::detail::output(const DirectiveType &line, ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "OUTPUT");
}

QSharedPointer<Node>
pas::operations::pepp::detail::scall(const DirectiveType &line, ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "SCALL");
}

QSharedPointer<Node>
pas::operations::pepp::detail::section(const DirectiveType &line, ST symTab) {
  throw std::logic_error("Unimplemented");
}

QSharedPointer<Node>
pas::operations::pepp::detail::uscall(const DirectiveType &line, ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "USCALL");
}

QSharedPointer<Node>
pas::operations::pepp::detail::word(const DirectiveType &line, ST symTab) {
  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    throw std::logic_error("Error");
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    throw std::logic_error("Error");
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  ret->set(generic::Directive{.value = u"WORD"_qs});
  ret->set(generic::Argument{.value = arg});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}
