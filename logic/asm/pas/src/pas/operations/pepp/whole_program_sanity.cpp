#include "./whole_program_sanity.hpp"
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/value/symbolic.hpp"
#include "pas/operations/generic/is.hpp"
#include "symbol/entry.hpp"

bool pas::ops::pepp::IsOSFeature::operator()(const ast::Node &node) {
  static const auto osDirectives = QSet<QString>{
      "BURN", "EXPORT", "IMPORT", "INPUT", "OUTPUT", "SCALL", "USCALL"};
  return ast::type(node).value == ast::generic::Type::Directive &&
         osDirectives.contains(node.get<ast::generic::Directive>().value);
}

bool pas::ops::pepp::hasOSFeatures(ast::Node &node) {
  pas::ops::generic::Exists<IsOSFeature> feats;
  ast::apply_recurse(node, feats);
  return feats.result;
}

void pas::ops::pepp::ErrorOnOSFeatures::operator()(ast::Node &node) {
  IsOSFeature feat;
  if (feat(node)) {
    if (node.has<ast::generic::Directive>()) {
      auto name = node.get<ast::generic::Directive>().value;
      auto message = pas::errors::pepp::illegalInUser.arg("." + name);
      ast::addError(node,
                    ast::generic::Message{
                        .severity = ast::generic::Message::Severity::Fatal,
                        .message = message});
    } else {
      throw std::logic_error("Unimplemented code path in ErrorsOnOSFeatures");
    }
  }
}

void pas::ops::pepp::errorOnOSFeatures(ast::Node &node) {
  ErrorOnOSFeatures visit;
  IsOSFeature is;
  ast::apply_recurse_if<void>(node, is, visit);
}

// Return true if there is an undefined symbolic argument.
// Also annotates the node with an appropriate error message.
bool annotateUndefinedArgument(pas::ast::Node &node,
                               pas::ast::value::Base *arg) {
  auto casted = dynamic_cast<pas::ast::value::Symbolic *>(arg);
  if (casted == nullptr)
    return false;
  else if (casted->symbol()->state == symbol::DefinitionState::kUndefined) {
    pas::ast::addError(node,
                       {.severity = pas::ast::generic::Message::Severity::Fatal,
                        .message = pas::errors::pepp::undefinedSymbol.arg(
                            casted->symbol()->name)});
    return true;
  } else
    return false;
}
void pas::ops::pepp::ErrorOnUndefinedSymbolicArgument::operator()(
    ast::Node &node) {
  if (node.has<ast::generic::Argument>())
    hadError |= annotateUndefinedArgument(
        node, node.get<ast::generic::Argument>().value.data());
  else if (node.has<ast::generic::ArgumentList>())
    for (auto arg : node.get<ast::generic::ArgumentList>().value)
      hadError |= annotateUndefinedArgument(node, arg.data());
}

bool pas::ops::pepp::errorOnUndefinedSymbolicArgument(ast::Node &node) {
  ErrorOnUndefinedSymbolicArgument visit;
  ast::apply_recurse(node, visit);
  return visit.hadError;
}
