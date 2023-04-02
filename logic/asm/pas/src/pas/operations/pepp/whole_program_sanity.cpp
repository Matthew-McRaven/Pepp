#include "./whole_program_sanity.hpp"

#include <pas/operations/generic/is.hpp>

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
