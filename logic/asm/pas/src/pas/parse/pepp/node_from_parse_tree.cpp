#include "./node_from_parse_tree.hpp"
#include "pas/ast/generic/attr_error.hpp"
#include "pas/ast/generic/attr_sec_flags.hpp"
#include "pas/ast/value/symbolic.hpp"
#include "pas/errors.hpp"
#include <pas/ast/generic/attr_argument.hpp>
#include <pas/ast/generic/attr_comment.hpp>
#include <pas/ast/generic/attr_directive.hpp>
#include <pas/ast/generic/attr_symbol.hpp>
#include <pas/ast/value/hexadecimal.hpp>
#include <pas/ast/value/identifier.hpp>
using ST = QSharedPointer<symbol::Table>;
using namespace pas::parse::pepp;
using namespace pas::ast;
using pas::ast::generic::Message;
using S = Message::Severity;
namespace EP = pas::errors::pepp;
using Error = pas::ast::generic::Error;
QSharedPointer<Node>
pas::parse::pepp::detail::addError(QSharedPointer<Node> node,
                                   pas::ast::generic::Message msg) {
  QList<Message> messages;
  if (node->has<generic::Error>())
    messages = node->get<generic::Error>().value;
  messages.push_back(msg);
  node->set(generic::Error{.value = messages});
  return node;
}

QSharedPointer<Node>
pas::parse::pepp::detail::gen_io_scall_extern(const DirectiveType &line,
                                              ST symTab, QString directive) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = directive});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (auto as_sym = dynamic_cast<value::Symbolic *>(arg.data());
      as_sym == nullptr)
    return addError(ret,
                    {.severity = S::Fatal, .message = EP::expectedSymbolic});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    return addError(ret, {.severity = S::Fatal,
                          .message = EP::noDefineSymbol.arg(directive)});

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
pas::parse::pepp::detail::align(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"ALIGN"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric() && isPow2(arg)))
    return addError(ret, {.severity = S::Fatal, .message = EP::alignPow2});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::parse::pepp::detail::ascii(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"ASCII"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!arg->isText())
    return addError(ret, {.severity = S::Fatal,
                          .message = EP::dotRequiresString.arg(u".ASCII"_qs)});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::parse::pepp::detail::block(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"BLOCK"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    return addError(ret,
                    {.severity = S::Fatal, .message = EP::expectedNumeric});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::parse::pepp::detail::burn(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"BURN"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (auto asHex = dynamic_cast<pas::ast::value::Hexadecimal *>(arg.data());
      asHex == nullptr)
    return addError(ret,
                    {.severity = S::Fatal, .message = EP::burnRequiresHex});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});
  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::parse::pepp::detail::byte(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"BYTE"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    return addError(ret,
                    {.severity = S::Fatal, .message = EP::expectedNumeric});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::parse::pepp::detail::end(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"END"_qs});

  if (line.args.size() != 0)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(0)});

  if (!line.symbol.empty())
    return addError(
        ret, {.severity = S::Fatal, .message = EP::noDefineSymbol.arg(1)});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<pas::ast::Node>
pas::parse::pepp::detail::equate(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"EQUATE"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    return addError(ret,
                    {.severity = S::Fatal, .message = EP::expectedNumeric});
  ret->set(generic::Argument{.value = arg});

  if (line.symbol.empty())
    return addError(
        ret, {.severity = S::Fatal, .message = EP::equateRequiresSymbol});
  ret->set(generic::SymbolDeclaration{
      .value = symTab->define(QString::fromStdString(line.symbol))});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<Node>
pas::parse::pepp::detail::_export(const DirectiveType &line, ST symTab) {
  return detail::gen_io_scall_extern(line, symTab, "EXPORT");
}

QSharedPointer<Node> pas::parse::pepp::detail::import(const DirectiveType &line,
                                                      ST symTab) {
  return detail::gen_io_scall_extern(line, symTab, "IMPORT");
}

QSharedPointer<Node> pas::parse::pepp::detail::input(const DirectiveType &line,
                                                     ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "INPUT");
}

QSharedPointer<Node> pas::parse::pepp::detail::output(const DirectiveType &line,
                                                      ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "OUTPUT");
}

QSharedPointer<Node> pas::parse::pepp::detail::scall(const DirectiveType &line,
                                                     ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "SCALL");
}

QSharedPointer<Node>
pas::parse::pepp::detail::section(const DirectiveType &line, ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"SECTION"_qs});

  auto args = detail::parse_arg(line, symTab, true);

  // TODO: Handle section flags (second argument).
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!arg->isText())
    return addError(ret,
                    {.severity = S::Fatal,
                     .message = EP::dotRequiresString.arg(u".SECTION"_qs)});
  ret->set(generic::Argument{.value = arg});
  ret->set(generic::SectionFlags{
      .value = {.R = 1, .W = 1, .X = 1}}); // Default to read/write/execute

  if (!line.symbol.empty())
    return addError(ret, {.severity = S::Fatal,
                          .message = EP::noDefineSymbol.arg(".SECTION")});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}

QSharedPointer<Node> pas::parse::pepp::detail::uscall(const DirectiveType &line,
                                                      ST symTab) {

  return detail::gen_io_scall_extern(line, symTab, "USCALL");
}

QSharedPointer<Node> pas::parse::pepp::detail::word(const DirectiveType &line,
                                                    ST symTab) {
  auto ret = QSharedPointer<pas::ast::Node>::create(
      generic::Type{.value = generic::Type::Directive});
  ret->set(generic::Directive{.value = u"WORD"_qs});

  auto args = detail::parse_arg(line, symTab);
  if (args.size() != 1)
    return addError(
        ret, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
  auto arg = args[0];
  if (!(arg->isFixedSize() && arg->isNumeric()))
    return addError(ret,
                    {.severity = S::Fatal, .message = EP::expectedNumeric});
  ret->set(generic::Argument{.value = arg});

  if (!line.symbol.empty())
    ret->set(generic::SymbolDeclaration{
        .value = symTab->define(QString::fromStdString(line.symbol))});

  if (line.hasComment)
    ret->set(generic::Comment{.value = QString::fromStdString(line.comment)});
  return ret;
}
