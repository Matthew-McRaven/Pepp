#pragma once
#include "../../../ast/node/nocode.hpp"
#include "../../parse/types_lines.hpp"
#include "../node/instruction.hpp"
#include "../node/instruction_a.hpp"
#include "../node/instruction_aaa.hpp"
#include "../node/instruction_r.hpp"
#include "../node/instruction_raaa.hpp"
#include "../node/instruction_u.hpp"
#include "../node/root.hpp"
#include "../node/types.hpp"
#include "./arg_from_parse_tree.hpp"

#include "pat/ast/node/directive.hpp"
#include "pat/ast/node/error.hpp"
#include "symbol/table.hpp"
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>
namespace pat::pep::ast::visitor {
template <typename ISA>

struct FromParseTree : public boost::static_visitor<node::ChildNode<ISA>> {
  QSharedPointer<symbol::Table> symTab;
  node::ChildNode<ISA> operator()(const parse::BlankType &line);
  node::ChildNode<ISA> operator()(const parse::CommentType &line);
  node::ChildNode<ISA> operator()(const parse::UnaryType &line);
  node::ChildNode<ISA> operator()(const parse::NonUnaryType &line);
  node::ChildNode<ISA> operator()(const parse::DirectiveType &line);
  node::ChildNode<ISA> operator()(const parse::MacroType &line);
  template <typename T> node::ChildNode<ISA> operator()(const T &line);
};

template <typename ISA>
QSharedPointer<pat::pep::ast::node::Root<ISA>>
toAST(const std::vector<parse::LineType> &lines) {
  auto root = QSharedPointer<pat::pep::ast::node::Root<ISA>>::create();
  root->setSymbolTable(QSharedPointer<symbol::Table>::create());
  QSharedPointer<pat::pep::ast::node::SectionGroup<ISA>> activeSection;
  auto visitor = FromParseTree<ISA>();
  auto createActive = [&]() {
    activeSection =
        QSharedPointer<pat::pep::ast::node::SectionGroup<ISA>>::create();
    activeSection->setSymbolTable(root->symbolTable()->addChild());
    root->addChild(activeSection);
    visitor.symTab = activeSection->getSymbolTable();
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
using namespace parse;
using namespace node;
template <typename ByteType>
QSharedPointer<pat::ast::node::Base> bytes_gen(const DirectiveType &line,
                                               ST symtab);
template <typename DirectiveNodeType>
QSharedPointer<pat::ast::node::Base>
single_argument_gen(const DirectiveType &line, ST symtab);
template <typename IOScallType>
QSharedPointer<pat::ast::node::Base> io_or_scall_gen(const DirectiveType &line,
                                                     ST symtab);
QSharedPointer<pat::ast::node::Base> align(const DirectiveType &line,
                                           ST symTab);
QSharedPointer<pat::ast::node::Base> ascii(const DirectiveType &line,
                                           ST symTab);
QSharedPointer<pat::ast::node::Base> block(const DirectiveType &line,
                                           ST symTab);
QSharedPointer<pat::ast::node::Base> burn(const DirectiveType &line, ST symTab);
QSharedPointer<pat::ast::node::Base> byte(const DirectiveType &line, ST symTab);
QSharedPointer<pat::ast::node::Base> end(const DirectiveType &line, ST symTab);
QSharedPointer<pat::ast::node::Base> equate(const DirectiveType &line,
                                            ST symTab);
QSharedPointer<pat::ast::node::Base> _export(const DirectiveType &line,
                                             ST symTab);
QSharedPointer<pat::ast::node::Base> import(const DirectiveType &line,
                                            ST symTab);
QSharedPointer<pat::ast::node::Base> input(const DirectiveType &line,
                                           ST symTab);
QSharedPointer<pat::ast::node::Base> output(const DirectiveType &line,
                                            ST symTab);
QSharedPointer<pat::ast::node::Base> scall(const DirectiveType &line,
                                           ST symTab);
QSharedPointer<pat::ast::node::Base> section(const DirectiveType &line,
                                             ST symTab);
QSharedPointer<pat::ast::node::Base> uscall(const DirectiveType &line,
                                            ST symTab);
QSharedPointer<pat::ast::node::Base> word(const DirectiveType &line, ST symTab);

} // namespace detail

template <typename ISA>
node::ChildNode<ISA>
FromParseTree<ISA>::operator()(const parse::BlankType &line) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  return QSharedPointer<pat::ast::node::Blank>::create(FileLocation(),
                                                       QWeakPointer<Base>());
}

template <typename ISA>
node::ChildNode<ISA>
FromParseTree<ISA>::operator()(const parse::CommentType &line) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  auto comment = QSharedPointer<pat::ast::node::Comment>::create(
      QString::fromStdString(line.comment), FileLocation(),
      QWeakPointer<Base>());
  comment->setIndent(pat::ast::node::Comment::IndentLevel::Left);
  comment->setConfig({.commentExpr = u";"_qs});
  return comment;
}

template <typename ISA>
node::ChildNode<ISA>
FromParseTree<ISA>::operator()(const parse::UnaryType &line) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  typename ISA::Mnemonic mnemonic =
      ISA::parseMnemonic(QString::fromStdString(line.identifier));
  QSharedPointer<node::Instruction<ISA>> instr;
  if (ISA::isUType(mnemonic))
    instr = QSharedPointer<node::Instruction_U<ISA>>::create(
        mnemonic, FileLocation(), QWeakPointer<Base>());
  else if (ISA::isRType(mnemonic))
    instr = QSharedPointer<node::Instruction_R<ISA>>::create(
        mnemonic, FileLocation(), QWeakPointer<Base>());
  else
    throw std::logic_error("Invalid mnemonic");

  if (line.hasComment)
    instr->setComment(QString::fromStdString(line.comment));
  if (!line.symbol.empty())
    instr->setSymbol(symTab->define(QString::fromStdString(line.symbol)));
  return instr;
}
template <typename ISA>
node::ChildNode<ISA>
FromParseTree<ISA>::operator()(const parse::NonUnaryType &line) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  // TODO: Capture argument;
  typename ISA::Mnemonic mnemonic =
      ISA::parseMnemonic(QString::fromStdString(line.identifier));
  auto addrMode = ISA::parseAddressingMode(QString::fromStdString(line.addr));
  QSharedPointer<node::Instruction<ISA>> instr;
  auto visit = visitor::ParseToArg();
  QSharedPointer<pat::ast::argument::Base> argument =
      line.arg.apply_visitor(visit);
  if (ISA::isAType(mnemonic)) {
    if (!ISA::isValidATypeAddressingMode(addrMode))
      throw std::logic_error("Invalid addr mode");
    instr = QSharedPointer<node::Instruction_A<ISA>>::create(
        mnemonic, addrMode, FileLocation(), QWeakPointer<Base>());

    if (auto result = node::Instruction_A<ISA>::validateArgument(argument);
        !result.valid)
      throw std::logic_error("Error");
    else
      dynamic_cast<node::Instruction_A<ISA> *>(instr.data())
          ->setArgument(argument);

  } else if (ISA::isAAAType(mnemonic)) {
    if (!ISA::isValidAAATypeAddressingMode(addrMode))
      throw std::logic_error("Invalid addr mode");
    instr = QSharedPointer<node::Instruction_AAA<ISA>>::create(
        mnemonic, addrMode, FileLocation(), QWeakPointer<Base>());

    if (auto result = node::Instruction_AAA<ISA>::validateArgument(argument);
        !result.valid)
      throw std::logic_error("Error");
    else
      dynamic_cast<node::Instruction_AAA<ISA> *>(instr.data())
          ->setArgument(argument);

  } else if (ISA::isRAAAType(mnemonic)) {
    if (!ISA::isValidRAAATypeAddressingMode(addrMode))
      throw std::logic_error("Invalid addr mode");
    instr = QSharedPointer<node::Instruction_RAAA<ISA>>::create(
        mnemonic, addrMode, FileLocation(), QWeakPointer<Base>());

    if (auto result = node::Instruction_RAAA<ISA>::validateArgument(argument);
        !result.valid)
      throw std::logic_error("Error");
    else
      dynamic_cast<node::Instruction_RAAA<ISA> *>(instr.data())
          ->setArgument(argument);

  } else
    throw std::logic_error("Invalid mnemonic");

  if (line.hasComment)
    instr->setComment(QString::fromStdString(line.comment));
  if (!line.symbol.empty())
    instr->setSymbol(symTab->define(QString::fromStdString(line.symbol)));
  return instr;
}
template <typename ISA>
node::ChildNode<ISA>
FromParseTree<ISA>::operator()(const parse::DirectiveType &line) {
  using convert_fn = std::function<QSharedPointer<pat::ast::node::Base>(
      const parse::DirectiveType &, QSharedPointer<symbol::Table>)>;

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
    auto ret = converter.value()(line, symTab);
    if (auto asDir = qSharedPointerCast<pat::ast::node::Directive>(ret);
        ret != nullptr)
      return asDir;
    else if (auto asErr = qSharedPointerCast<pat::ast::node::Error>(ret);
             ret != nullptr)
      return asErr;
    else
      throw std::logic_error("bad type");
  }
  throw std::logic_error("No match");
}
template <typename ISA>
node::ChildNode<ISA>
FromParseTree<ISA>::operator()(const parse::MacroType &line) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  return QSharedPointer<pat::ast::node::Blank>::create(FileLocation(),
                                                       QWeakPointer<Base>());
}
template <typename ISA>
template <typename T>
node::ChildNode<ISA> FromParseTree<ISA>::operator()(const T &line) {
  throw std::logic_error("Invalid T");
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  return QSharedPointer<pat::ast::node::Blank>::create(FileLocation(),
                                                       QWeakPointer<Base>());
}
} // namespace pat::pep::ast::visitor

template <typename ByteType>
QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::bytes_gen(const DirectiveType &line,
                                          ST symTab) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  auto visitor = visitor::ParseToArg();
  visitor.symTab = symTab;
  QList<QSharedPointer<const pat::ast::argument::Base>> arguments;
  for (const auto &arg : line.args) {
    auto shared = arg.apply_visitor(visitor);
    if (visitor.error)
      throw std::logic_error("error");
    arguments.push_back(shared);
  }
  if (auto result = ByteType::validate_argument(arguments); result.valid) {
    auto ret = QSharedPointer<ByteType>::create(arguments, FileLocation(),
                                                QWeakPointer<Base>());
    if (line.hasComment)
      ret->setComment(QString::fromStdString(line.comment));
    if (!line.symbol.empty())
      ret->setSymbol(symTab->define(QString::fromStdString(line.symbol)));
    return ret;
  } else
    throw std::logic_error("error");
};

template <typename DirectiveNodeType>
QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::single_argument_gen(const DirectiveType &line,
                                                    ST symTab) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  auto visitor = visitor::ParseToArg();
  visitor.symTab = symTab;
  if (auto count = line.args.size(); count != 1)
    throw std::logic_error("Requires 1 arg");
  else {
    auto shared = line.args[0].apply_visitor(visitor);
    if (visitor.error)
      throw std::logic_error("error");
    if (auto result = DirectiveNodeType::validate_argument(shared);
        result.valid) {
      auto ret = QSharedPointer<DirectiveNodeType>::create(
          shared, FileLocation(), QWeakPointer<Base>());
      if (line.hasComment)
        ret->setComment(QString::fromStdString(line.comment));
      if (!line.symbol.empty())
        ret->setSymbol(symTab->define(QString::fromStdString(line.symbol)));
      return ret;
    } else
      throw std::logic_error("error");
  };
}

template <typename IOScallType>
QSharedPointer<pat::ast::node::Base>
pat::pep::ast::visitor::detail::io_or_scall_gen(const DirectiveType &line,
                                                ST symTab) {
  using ::pat::ast::node::Base;
  using ::pat::ast::node::FileLocation;
  auto visitor = visitor::ParseToArg();
  visitor.symTab = symTab;
  if (auto count = line.args.size(); count != 1)
    throw std::logic_error("Requires 1 arg");
  else {
    auto base = line.args[0].apply_visitor(visitor);
    auto casted = qSharedPointerCast<pat::ast::argument::Symbolic>(base);
    if (visitor.error)
      throw std::logic_error("error");
    else if (casted == nullptr)
      throw std::logic_error("invalid argument type");
    if (auto result = IOScallType::validate_argument(casted); result.valid) {
      auto ret = QSharedPointer<IOScallType>::create(casted, FileLocation(),
                                                     QWeakPointer<Base>());
      if (line.hasComment)
        ret->setComment(QString::fromStdString(line.comment));
      if (line.symbol.empty())
        throw std::logic_error("can't have a symbol");
      return ret;
    } else
      throw std::logic_error("error");
  };
}

// namespace pat::pep::ast::visitor

/*
 * template <typename T>
QSharedPointer<Root<T>> toQST(std::vector<ast::Line> &lines,
                              QSharedPointer<symbol::Table> rootTable) {
  auto root = QSharedPointer<Root<T>>::create();
  root->table = rootTable;
  QSharedPointer<SectionGroup<T>> activeSectionGroup;
  auto createSectionGroup = [&](QString name) {
    activeSectionGroup = QSharedPointer<SectionGroup<T>>::create();
    activeSectionGroup->table = root->table->addChild();
    root->children.append(activeSectionGroup);
    // TODO: assign section name, properties
  };
  createSectionGroup("default");
  for (const auto &line : lines) {
    auto asQST = line.apply_visitor(
        ConstructionVisitor<T>({.active = activeSectionGroup->tale}));
    if (auto *asPtr = boost::get<ast::Directive>(&asQST);
        asPtr && isSectionDirective(*asPtr)) {
      // TODO: get the right section name and properties
      createSectionGroup("new");
    }
    activeSectionGroup->children->append(asQST);
  }
}
*/
