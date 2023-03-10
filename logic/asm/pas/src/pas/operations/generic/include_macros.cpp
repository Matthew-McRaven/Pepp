#include "include_macros.hpp"
#include "is.hpp"
#include "macro/macro.hpp"
#include "macro/registered.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/ast/generic/attr_macro.hpp"
#include "pas/ast/node.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/driver/common.hpp"
#include "pas/errors.hpp"
#include "pas/operations/generic/errors.hpp"

void appendError(pas::ast::Node &node, QString message) {
  using namespace pas::ast::generic;
  Error err;
  if (node.has<Error>())
    err = node.get<Error>();
  err.value.push_back(
      Message{.severity = Message::Severity::Fatal, .message = message});
  node.set(err);
}
bool pas::ops::generic::IncludeMacros::operator()(ast::Node &node) {
  // Node should be a macro
  if (!node.has<ast::generic::Macro>()) {
    appendError(node, errors::pepp::expectedAMacro);
    return false;
  }
  auto macroName = node.get<ast::generic::Macro>().value;
  QStringList args = {};
  if (node.has<ast::generic::ArgumentList>()) {
    for (auto &arg : node.get<ast::generic::ArgumentList>().value)
      args.push_back(arg->string());
  } else if (node.has<ast::generic::Argument>())
    args.push_back(node.get<ast::generic::Argument>().value->string());

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
    appendError(node, errors::pepp::macroWrongArity.arg(macroName)
                          .arg(macroContents->argCount())
                          .arg(args.size()));
    return false;
  }
  // Perform macro arg substitution on body.
  auto macroText = macroContents->body();
  for (int it = 0; it < args.size(); it++)
    macroText.replace(u"%"_qs + QString::number(it), args[it]);

  // Function handles parenting macroText's nodes as node's children.
  // Parent/child relationships also established.
  auto converted = convertFn(macroText, node.sharedFromThis());

  if (converted.hadError) {
    for (auto &error : converted.errors)
      appendError(node, error);
    return false;
  }

  popMacroInvocation(macroInvoke);
  return true;
}

bool pas::ops::generic::IncludeMacros::pushMacroInvocation(
    MacroInvocation invoke) {
  if (_chain.contains(invoke))
    return false;
  _chain.insert(invoke);
  return true;
}

void pas::ops::generic::IncludeMacros::popMacroInvocation(
    MacroInvocation invoke) {
  _chain.remove(invoke);
}

bool pas::ops::generic::includeMacros(
    ast::Node &root,
    std::function<pas::driver::ParseResult(QString, QSharedPointer<ast::Node>)>
        convertFn,
    QSharedPointer<macro::Registry> registry) {
  static auto isMacro = pas::ops::generic::isMacro();
  auto convert = IncludeMacros();
  convert.convertFn = convertFn;
  convert.registry = registry;
  ast::apply_recurse_if(root, isMacro, convert);
  auto errors = pas::ops::generic::CollectErrors();
  ast::apply_recurse(root, errors);
  return errors.errors.size() == 0;
}
