#include "include_macros.hpp"
#include "is.hpp"
#include "macro/macro.hpp"
#include "macro/registered.hpp"
#include "macro/registry.hpp"
#include "pas/ast/generic/attr_children.hpp"
#include "pas/ast/generic/attr_macro.hpp"
#include "pas/ast/node.hpp"
#include "pas/errors.hpp"
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
  // TODO: Add error messages when returning.
  // Node should be a macro
  if (!node.has<ast::generic::Macro>())
    return false;
  auto macroName = node.get<ast::generic::Macro>().value;
  QStringList args = {};
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

  auto converted = convertFn(macroText);

  // Update parent/child relationships
  node.set(ast::generic::Children{.value = converted});
  for (auto &n : converted)
    ast::setParent(*n, node.sharedFromThis());

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
    std::function<QList<QSharedPointer<ast::Node>>(QString)> convertFn,
    QSharedPointer<macro::Registry> registry) {
  static auto isMacro = pas::ops::generic::isMacro();
  auto convert = IncludeMacros();
  convert.convertFn = convertFn;
  convert.registry = registry;
  ast::apply_recurse_if(root, isMacro, convert);
  // TODO: Catch parsing errors;
  return true;
}
