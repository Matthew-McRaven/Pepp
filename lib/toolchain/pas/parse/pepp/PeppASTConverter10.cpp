#include "PeppASTConverter10.h"
#include "toolchain/pas/string_utils.hpp"

#undef emit
#include "toolchain/pas/ast/generic/attr_argument.hpp"
#include "toolchain/pas/ast/generic/attr_comment.hpp"
#include "toolchain/pas/ast/generic/attr_comment_indent.hpp"
#include "toolchain/pas/ast/generic/attr_directive.hpp"
#include "toolchain/pas/ast/generic/attr_location.hpp"
#include "toolchain/pas/ast/generic/attr_macro.hpp"
#include "toolchain/pas/ast/generic/attr_symbol.hpp"
#include "toolchain/pas/ast/node.hpp"
#include "toolchain/pas/ast/pepp/attr_addr.hpp"
#include "toolchain/pas/ast/pepp/attr_instruction.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/ast/value/character.hpp"
#include "toolchain/pas/ast/value/decimal.hpp"
#include "toolchain/pas/ast/value/hexadecimal.hpp"
#include "toolchain/pas/ast/value/string.hpp"
#include "toolchain/pas/ast/value/symbolic.hpp"
#include "toolchain/pas/errors.hpp"
#include "toolchain/symbol/table.hpp"
#include "bts/bitmanip/strings.hpp"
#include "bts/isa/pep/pep10.hpp"

using namespace pas::ast;
namespace {
bool isPow2_duplicate(QSharedPointer<pas::ast::value::Base> arg) {
  quint64 val = 0;
  arg->value(bits::span<quint8>{reinterpret_cast<quint8 *>(&val), 8}, bits::hostOrder());
  auto val_log2 = log2(val);
  return ceil(val_log2) == floor(val_log2);
}

QString errorFromWordString(QSharedPointer<value::Base> arg) {
  if (dynamic_cast<value::Hexadecimal *>(arg.data()) != nullptr) return pas::errors::pepp::hexTooBig2;
  else if (dynamic_cast<value::ShortString *>(arg.data()) != nullptr) return pas::errors::pepp::strTooLong2;
  else return pas::errors::pepp::decTooBig2;
}

QString errorFromByteString(QSharedPointer<value::Base> arg) {
  if (dynamic_cast<value::Hexadecimal *>(arg.data()) != nullptr) return pas::errors::pepp::hexTooBig1;
  else if (dynamic_cast<value::ShortString *>(arg.data()) != nullptr) return pas::errors::pepp::strTooLong1;
  else return pas::errors::pepp::decTooBig1;
}

void addBlank(QSharedPointer<Node> parent) {
  auto bl = QSharedPointer<Node>::create(generic::Type{.value = generic::Type::Blank});
  addChild(*parent, bl);
}
} // namespace
parse::PeppASTConverter::PeppASTConverter(QSharedPointer<pas::ast::Node> parent)
    : _blockInfo{.symTab = nullptr, .parent = parent} {}

std::any parse::PeppASTConverter::visitProg(PeppParser::ProgContext *context) {
  if (_blockInfo.parent == nullptr) {
    static const generic::Type structuralType = {.value = generic::Type::Structural};
    _blockInfo.parent = QSharedPointer<Node>::create(structuralType);
    auto st = QSharedPointer<symbol::Table>::create(2);
    generic::SymbolTable symtab = {.value = st};
    _blockInfo.parent->set(symtab);
  }
  QSharedPointer<Node> parent = _blockInfo.parent;
  _blockInfo.symTab = parent->get<generic::SymbolTable>().value;

  bool firstLine = true;
  int lineNo = 0;
  for (auto it = 0; it < context->children.size(); it++) {
    auto child = context->children[it];
    // Clear line-specific meta-info
    _lineInfo = {};
    // ret will only be empty if we immediately visited a terminal.
    // The only terminals reachable from prog are newlines and EOF.
    if (antlr4::tree::TerminalNode::is(child)) {
      auto typedChild = antlrcpp::downCast<antlr4::tree::TerminalNode *>(child);
      auto token = typedChild->getSymbol();
      if (token->getType() == PeppParser::EOF) break;
      else if (token->getType() == PeppParser::NEWLINE) {
        lineNo++;
        addBlank(parent);
      }
    } else {
      auto node = std::any_cast<QSharedPointer<Node>>(this->visit(child));
      node->set(generic::SourceLocation{.value = {.line = lineNo++, .valid = true}});
      addChild(*parent, node);
      // Eat the newline or EOF that always follows an expression.
      it++;
    }
    firstLine = false;
  }
  if (firstLine) addBlank(parent);
  return parent;
}

std::any parse::PeppASTConverter::visitNonUnaryInstruction(PeppParser::NonUnaryInstructionContext *context) {
  if (auto id = context->IDENTIFIER(0); id) _lineInfo.identifier = id->getText();
  auto children = context->children;
  // 0'th item is always the mnemonic
  if (auto child = children[0]; antlr4::tree::TerminalNode::is(child)) {
    auto typedChild = antlrcpp::downCast<antlr4::tree::TerminalNode *>(child);
    auto token = typedChild->getSymbol();
    if (token->getType() == PeppParser::PLACEHOLDER_MACRO || token->getType() == PeppParser::IDENTIFIER) {
      _lineInfo.identifier = typedChild->getText();
    }
  }

  // Next item is always operand, which is an argument.
  auto operand = visit(children[1]);
  _lineInfo.arguments.push_back(std::any_cast<QSharedPointer<value::Base>>(operand));

  // If there is a comma at 2, then there is an addressing mode at 3.
  if (!context->COMMA()) {
  } else if (auto child = children[3]; antlr4::tree::TerminalNode::is(child)) {
    auto typedChild = antlrcpp::downCast<antlr4::tree::TerminalNode *>(child);
    auto token = typedChild->getSymbol();
    if (token->getType() == PeppParser::PLACEHOLDER_MACRO || token->getType() == PeppParser::IDENTIFIER) {
      _lineInfo.addr_mode = typedChild->getText();
    }
  }

  return std::any();
}

std::any parse::PeppASTConverter::visitUnaryInstruction(PeppParser::UnaryInstructionContext *context) {
  if (auto id = context->IDENTIFIER(); id) _lineInfo.identifier = id->getText();
  return std::any();
}

std::any parse::PeppASTConverter::visitDirective(PeppParser::DirectiveContext *context) {
  // Needed to recursively parse arguments.
  visitChildren(context);

  _lineInfo.identifier = context->DOT_IDENTIFIER()->getText();
  // Remove . from directive name.
  _lineInfo.identifier->erase(0, 1);
  return std::any();
}

std::any parse::PeppASTConverter::visitInvoke_macro(PeppParser::Invoke_macroContext *context) {
  // Needed to recursively parse arguments.
  visitChildren(context);

  _lineInfo.identifier = context->AT_IDENTIFIER()->getText();
  _lineInfo.identifier->erase(0, 1); // Remove @ from macro name.
  return std::any();
}

std::any parse::PeppASTConverter::visitSymbol(PeppParser::SymbolContext *context) {
  auto text = context->children[0]->getText();
  // Remove colon from symbol
  text.pop_back();
  this->_lineInfo.symbol = text;
  return std::any();
}

std::any parse::PeppASTConverter::visitInstructionLine(PeppParser::InstructionLineContext *context) {
  using S = pas::ast::generic::Message::Severity;
  namespace EP = pas::errors::pepp;
  using pas::ast::generic::Type;
  using ISA = isa::Pep10;

  visitChildren(context);
  auto ret = QSharedPointer<Node>::create(Type{.value = Type::Instruction});
  if (_lineInfo.symbol.has_value()) {
    auto symbol = _blockInfo.symTab->define(QString::fromStdString(*_lineInfo.symbol));
    ret->set(generic::SymbolDeclaration{.value = symbol});
  }

  // BUG: instr will remain uninitialized if mnemonic is PLACEHOLDER_MACRO and the invalid
  // mnemonic path is removed.
  ISA::Mnemonic instr = ISA::parseMnemonic(QString::fromStdString(*_lineInfo.identifier));
  if (instr != ISA::Mnemonic::INVALID) ret->set(pepp::Instruction<isa::Pep10>{instr});
  else return addError(ret, {.severity = S::Fatal, .message = EP::invalidMnemonic});

  // If there are arguments, insert them into AST after check that args are <= 2 bytes.
  if (_lineInfo.arguments.size() == 1) {
    auto operand = _lineInfo.arguments[0];
    if (!(operand->isFixedSize() && operand->isNumeric()))
      return addError(ret, {.severity = S::Fatal, .message = EP::expectedNumeric});
    else if (operand->requiredBytes() > 2)
      return addError(ret, {.severity = S::Fatal, .message = errorFromWordString(operand)});
    ret->set(generic::Argument{.value = operand});
  }

  // Don't analyze addressing modes for unary mnemonics.
  if (ISA::isMnemonicUnary(instr))
    ;
  // BUG: Will not work when mnemonic is a PLACEHOLDER_MACRO.
  // Validate addressing mode is appropriate for instruction.
  // Triggered when a non-branch instruction is missing an addressing mode.
  else if (!_lineInfo.addr_mode.has_value() && ISA::requiresAddressingMode(instr))
    return addError(ret, {.severity = S::Fatal, .message = EP::requiredAddrMode});
  // Assign default addressing mode for BR-type mnemonics if it is not present
  else if (!_lineInfo.addr_mode.has_value())
    ret->set(pepp::AddressingMode<ISA>{.value = ISA::defaultAddressingMode(instr)});
  // Triggered when an instruction is not in the valid addressing mode set, like "p".
  else if (auto addr = ISA::parseAddressingMode(QString::fromStdString(*_lineInfo.addr_mode));
           addr == ISA::AddressingMode::INVALID)
    return addError(ret, {.severity = S::Fatal, .message = EP::illegalAddrMode});
  // Triggered when an addressing mode doesn't work with an instruction, like sfx with br, and i with stwa.
  else if ((ISA::isAType(instr) && !ISA::isValidATypeAddressingMode(instr, addr)) ||
           (ISA::isAAAType(instr) && !ISA::isValidAAATypeAddressingMode(instr, addr)) ||
           (ISA::isRAAAType(instr) && !ISA::isValidRAAATypeAddressingMode(instr, addr)))
    return addError(ret, {.severity = S::Fatal, .message = EP::illegalAddrMode});
  else ret->set(pepp::AddressingMode<ISA>{.value = addr});

  if (auto comment = context->COMMENT(); comment) {
    auto item = generic::Comment{.value = QString::fromStdString(comment->getText().substr(1))};
    ret->set(item);
  }

  return ret;
}

std::any parse::PeppASTConverter::visitDirectiveLine(PeppParser::DirectiveLineContext *context) {
  using S = pas::ast::generic::Message::Severity;
  namespace EP = pas::errors::pepp;
  using pas::ast::generic::Type;

  visitChildren(context);
  auto ret = QSharedPointer<Node>::create(Type{.value = Type::Directive});

  using convert_fn = void (PeppASTConverter::*)(QSharedPointer<Node>, PeppParser::DirectiveLineContext *);
  using namespace std::placeholders;
  // Must pass this manually, otherwise the first instance of our AST converter will be bound into the map.
  static QMap<QString, convert_fn> converters = {
      {"ALIGN", &PeppASTConverter::align},   {"ASCII", &PeppASTConverter::ascii},
      {"BLOCK", &PeppASTConverter::block},   {"BURN", &PeppASTConverter::burn},
      {"BYTE", &PeppASTConverter::byte},     {"END", &PeppASTConverter::end},
      {"EQUATE", &PeppASTConverter::equate}, {"EXPORT", &PeppASTConverter::_export},
      {"IMPORT", &PeppASTConverter::import}, {"INPUT", &PeppASTConverter::input},
      {"OUTPUT", &PeppASTConverter::output}, {"ORG", &PeppASTConverter::org},
      {"SCALL", &PeppASTConverter::scall},   {"SECTION", &PeppASTConverter::section},
      {"WORD", &PeppASTConverter::word},
  };

  auto identifier = *_lineInfo.identifier;
  auto asQString = QString::fromStdString(identifier).toUpper();
  generic::Directive directive = {.value = asQString};
  ret->set(directive);

  if (auto converter = converters.find(asQString); converter != converters.end()) {
    // Perform operations common to all dot commands here to prevent redundant code in helper functions.
    if (_lineInfo.symbol.has_value()) {
      auto symbol = _blockInfo.symTab->define(QString::fromStdString(*_lineInfo.symbol));
      ret->set(generic::SymbolDeclaration{.value = symbol});
    }
    if (auto comment = context->COMMENT(); comment) {
      auto item = generic::Comment{.value = QString::fromStdString(comment->getText().substr(1))};
      ret->set(item);
    }

    // If a dot command does not allow the attributes set above, they will need to add an error.
    convert_fn fn = converter.value();
    // Both sets of parens are syntactically necessary.
    (this->*fn)(ret, context);
  } else {
    // Triggered when the dot command is not in the above map.
    return addError(ret, {.severity = S::Fatal, .message = EP::invalidDirective});
  }

  return ret;
}

std::any parse::PeppASTConverter::visitMacroInvokeLine(PeppParser::MacroInvokeLineContext *context) {
  using S = pas::ast::generic::Message::Severity;
  namespace EP = pas::errors::pepp;
  using pas::ast::generic::Type;

  visitChildren(context);

  auto ret = QSharedPointer<Node>::create(Type{.value = Type::MacroInvoke});
  auto identifier = *_lineInfo.identifier;
  auto asQString = QString::fromStdString(identifier);
  generic::Macro macro = {.value = asQString};
  ret->set(macro);

  // Must share root symbol table until we have ability to limit symbol visibility.
  ret->set(generic::SymbolTable{.value = _blockInfo.symTab});

  if (_lineInfo.symbol.has_value()) {
    auto symbol = _blockInfo.symTab->define(QString::fromStdString(*_lineInfo.symbol));
    ret->set(generic::SymbolDeclaration{.value = symbol});
  }
  if (auto comment = context->COMMENT(); comment) {
    auto item = generic::Comment{.value = QString::fromStdString(comment->getText().substr(1))};
    ret->set(item);
  }
  QList<QSharedPointer<value::Base>> args;
  for (auto arg : _lineInfo.arguments) args.push_back(arg);
  ret->set(generic::ArgumentList{.value = args});

  return ret;
}

std::any parse::PeppASTConverter::visitCommentLine(PeppParser::CommentLineContext *context) {
  auto ret = QSharedPointer<Node>::create(generic::Type{.value = pas::ast::generic::Type::Comment});
  // remove semicolon from comment
  auto comment_text = context->COMMENT()->getText().substr(1);
  ret->set(generic::Comment{.value = QString::fromStdString(comment_text)});
  ret->set(generic::CommentIndent{.value = generic::CommentIndent::Level::Left});
  return ret;
}

std::any parse::PeppASTConverter::visitArgument(PeppParser::ArgumentContext *context) {
  // Must return via type-erased "ret", since std::any will not allow us to easily cast
  // from derived to base within any_cast.
  QSharedPointer<value::Base> ret = nullptr;
  if (auto tok = context->CHARACTER(); tok) {
    // Remove quotes from char
    auto text = tok->getText();
    text = text.substr(1, text.length() - 2);
    auto asQString = QString::fromStdString(text);
    ret = QSharedPointer<value::Character>::create(asQString);
  } else if (tok = context->STRING(); tok) {
    // Remove quotes from string
    auto text = tok->getText();
    text = text.substr(1, text.length() - 2);
    auto asQString = QString::fromStdString(text);
    if (auto length = bits::escapedStringLength(asQString); length <= 2)
      ret = QSharedPointer<value::ShortString>::create(asQString, length, bits::Order::BigEndian);
    else ret = QSharedPointer<value::LongString>::create(asQString, bits::Order::BigEndian);
  } else if (tok = context->IDENTIFIER(); tok) {
    auto asQString = QString::fromStdString(tok->getText());
    auto asSymbol = _blockInfo.symTab->reference(asQString);
    ret = QSharedPointer<value::Symbolic>::create(asSymbol);
  } else if (tok = context->HEXADECIMAL(); tok) {
    bool okay = 0;
    auto value = QString::fromStdString(tok->getText()).toULongLong(&okay, 16);
    if (!okay) throw std::logic_error("Hex didn't convert");
    ret = QSharedPointer<value::Hexadecimal>::create(value, 2);
  } else if (tok = context->SIGNED_DECIMAL(); tok) {
    bool okay = 0;
    auto value = QString::fromStdString(tok->getText()).toLongLong(&okay, 10);
    if (!okay) throw std::logic_error("Signed dec didn't convert");
    ret = QSharedPointer<value::SignedDecimal>::create(value, 2);
  } else if (tok = context->UNSIGNED_DECIMAL(); tok) {
    bool okay = 0;
    auto value = QString::fromStdString(tok->getText()).toLongLong(&okay, 10);
    if (!okay) throw std::logic_error("Signed dec didn't convert");
    ret = QSharedPointer<value::UnsignedDecimal>::create(value, 2);
  } else if (tok = context->PLACEHOLDER_MACRO(); tok) {
    throw std::logic_error("Macro placeholder not implemented");
  }
  return ret;
}

std::any parse::PeppASTConverter::visitArgument_list(PeppParser::Argument_listContext *context) {
  for (auto child : context->children) {
    // Non-terminals must be arguments, per parser grammar.
    // Ignore terminals, as they must all be COMMA
    if (child->getTreeType() != antlr4::tree::ParseTreeType::TERMINAL) {
      auto ret = this->visit(child);
      using T = QSharedPointer<value::Base>;
      if (ret.type() != typeid(T)) {
        int x = 12;
        throw std::logic_error("WTF");
      }
      auto casted = std::any_cast<T>(ret);
      _lineInfo.arguments.push_back(casted);
    }
  }
  return std::any();
}
using S = pas::ast::generic::Message::Severity;
namespace EP = pas::errors::pepp;

void parse::PeppASTConverter::align(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  if (!(arg->isFixedSize() && arg->isNumeric() && isPow2_duplicate(arg)))
    addError(node, {.severity = S::Fatal, .message = EP::alignPow2});
  else if (arg->requiredBytes() > 2) addError(node, {.severity = S::Fatal, .message = errorFromWordString(arg)});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::ascii(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  if (!arg->isText()) addError(node, {.severity = S::Fatal, .message = EP::dotRequiresString.arg(".ASCII")});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::block(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  // Triggers when you pass an argument that is a string that is too long.
  if (!(arg->isFixedSize() && arg->isNumeric()) || arg->isText())
    addError(node, {.severity = S::Fatal, .message = EP::expectedNumeric});
  else if (arg->isSigned()) addError(node, {.severity = S::Fatal, .message = EP::decUnsigned2});
  else if (arg->requiredBytes() > 2) addError(node, {.severity = S::Fatal, .message = errorFromWordString(arg)});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::burn(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  // Triggers when the argument is not a hex constant.
  if (auto asHex = dynamic_cast<pas::ast::value::Hexadecimal *>(arg.data()); asHex == nullptr)
    addError(node, {.severity = S::Fatal, .message = EP::requiresHex.arg(".BURN")});
  else if (arg->requiredBytes() > 2) addError(node, {.severity = S::Fatal, .message = errorFromWordString(arg)});
  else if (node->has<generic::SymbolDeclaration>())
    addError(node, {.severity = S::Fatal, .message = EP::noDefineSymbol.arg(".BURN")});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::byte(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  // Triggers when you pass an argument that is a string that is too long.
  if (arg->isText() && arg->size() > 1) addError(node, {.severity = S::Fatal, .message = EP::strTooLong1});
  else if (!(arg->isFixedSize() && arg->isNumeric()))
    addError(node, {.severity = S::Fatal, .message = EP::expectedNumeric});
  else if (arg->requiredBytes() > 1) addError(node, {.severity = S::Fatal, .message = errorFromByteString(arg)});
  else {
    arg->resize(1);
    node->set(generic::Argument{.value = arg});
  }
}

void parse::PeppASTConverter::end(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 0) addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(0)});
  else if (node->has<generic::SymbolDeclaration>())
    addError(node, {.severity = S::Fatal, .message = EP::noDefineSymbol.arg(".END")});
}

void parse::PeppASTConverter::equate(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  // Triggers when you pass an argument that is a string that is too long.
  if (arg->isText() && arg->size() > 2) addError(node, {.severity = S::Fatal, .message = EP::strTooLong2});
  else if (!(arg->isFixedSize() && arg->isNumeric()))
    addError(node, {.severity = S::Fatal, .message = EP::expectedNumeric});
  else if (arg->requiredBytes() > 2) addError(node, {.severity = S::Fatal, .message = errorFromWordString(arg)});
  else if (!node->has<generic::SymbolDeclaration>())
    addError(node, {.severity = S::Fatal, .message = EP::equateRequiresSymbol});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::_export(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  io_scall_helper(node, context, "EXPORT");
}

void parse::PeppASTConverter::import(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  io_scall_helper(node, context, "IMPORT");
}

void parse::PeppASTConverter::input(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  io_scall_helper(node, context, "INPUT");
}

void parse::PeppASTConverter::output(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  io_scall_helper(node, context, "OUTPUT");
}

void parse::PeppASTConverter::org(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  // Triggers when the argument is not a hex constant.
  if (auto asHex = dynamic_cast<pas::ast::value::Hexadecimal *>(arg.data()); asHex == nullptr)
    addError(node, {.severity = S::Fatal, .message = EP::requiresHex.arg(".ORG")});
  else if (arg->requiredBytes() > 2) addError(node, {.severity = S::Fatal, .message = errorFromWordString(arg)});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::scall(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  io_scall_helper(node, context, "SCALL");
}

void parse::PeppASTConverter::section(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {

  if (_lineInfo.arguments.size() == 0 || _lineInfo.arguments.size() > 2) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNMArguments.arg(1, 2)});
    return;
  }

  auto arg = _lineInfo.arguments[0];
  // Triggers when the argument is not a string
  if (!arg->isText()) addError(node, {.severity = S::Fatal, .message = EP::dotRequiresString.arg(".SECTION")});
  else if (node->has<generic::SymbolDeclaration>())
    addError(node, {.severity = S::Fatal, .message = EP::noDefineSymbol.arg(".SECTION")});
  else if (_lineInfo.arguments.size() == 2) {
    auto flagArg = _lineInfo.arguments[1];
    if (!flagArg->isText()) addError(node, {.severity = S::Fatal, .message = EP::sectionFlagsString});
    else node->set(generic::ArgumentList{.value = {arg, flagArg}});
  } else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::word(QSharedPointer<pas::ast::Node> node, PeppParser::DirectiveLineContext *context) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  // Triggers when you pass an argument that is a string that is too long.
  if (arg->isText() && arg->size() > 2) addError(node, {.severity = S::Fatal, .message = EP::strTooLong2});
  else if (!(arg->isFixedSize() && arg->isNumeric()))
    addError(node, {.severity = S::Fatal, .message = EP::expectedNumeric});
  else if (arg->requiredBytes() > 2) addError(node, {.severity = S::Fatal, .message = errorFromWordString(arg)});
  else node->set(generic::Argument{.value = arg});
}

void parse::PeppASTConverter::io_scall_helper(QSharedPointer<pas::ast::Node> node,
                                              PeppParser::DirectiveLineContext *context, QString name) {
  if (_lineInfo.arguments.size() != 1) {
    addError(node, {.severity = S::Fatal, .message = EP::expectNArguments.arg(1)});
    return;
  }

  auto arg = _lineInfo.arguments.at(0);
  if (auto as_sym = dynamic_cast<value::Symbolic *>(arg.data()); as_sym == nullptr)
    addError(node, {.severity = S::Fatal, .message = EP::expectedSymbolic});
  else if (node->has<generic::SymbolDeclaration>())
    addError(node, {.severity = S::Fatal, .message = EP::noDefineSymbol.arg("." + name)});
  else node->set(generic::Argument{.value = arg});
}
