#include "./register_system_calls.hpp"
#include "macro/macro.hpp"
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"

// TODO: Determine if 1-indexed of 0-indexed.
const QString unarySCallMacro = "LDWT %1\nUSCALL\n";
const QString nonunarySCallMacro = "LDWT %1\nSCALL %%1,%%2\n";
using pas::ast::generic::Message;

bool pas::ops::pepp::RegisterSystemCalls::operator()(ast::Node &node) {
  auto macroKind = node.get<ast::generic::Directive>().value;
  QSharedPointer<macro::Parsed> parsed = {};

  // Validate that directive has correct argument types, then construct correct
  // macro kind (unary/non-unary)
  if (!node.has<ast::generic::Argument>()) {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal,
                         .message = u"%1 missing argument."_qs.arg(macroKind)});
  } else if (auto argument = node.get<ast::generic::Argument>().value;
             !argument->isIdentifier()) {
    addedError = true;
    ast::addError(
        node,
        {.severity = Message::Severity::Fatal,
         .message = u"%1 expected a identifier argument."_qs.arg(macroKind)});
  } else if (macroKind.toUpper() == "SCALL") {
    auto name = argument->string();
    parsed = QSharedPointer<macro::Parsed>::create(
        name, 2, nonunarySCallMacro.arg(name) + "%1, %2\n", "pep/10");
  } else if (macroKind.toUpper() == "USCALL") {
    auto name = argument->string();
    parsed = QSharedPointer<macro::Parsed>::create(
        name, 0, unarySCallMacro.arg(name), "pep/10");
  } else {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal,
                         .message = u"Unspecified error."_qs});
  }

  // Attempt to register macro, and propogate error if it already exists.
  if (!parsed.isNull() &&
      registry->registerMacro(macro::types::System, parsed) == nullptr) {
    addedError = true;
    ast::addError(node, {.severity = Message::Severity::Fatal,
                         .message = u"Duplicate system call."_qs});
  }
  return addedError;
}

bool pas::ops::pepp::registerSystemCalls(
    ast::Node &node, QSharedPointer<macro::Registry> registry) {
  auto is = pas::ops::generic::isDirective();
  auto visit = RegisterSystemCalls();
  visit.registry = registry;
  ast::apply_recurse_if(node, is, visit);
  return visit.addedError;
}
