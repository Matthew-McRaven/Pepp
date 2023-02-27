#include "./is.hpp"
#include "pas/ast/generic/attr_sec.hpp"

#include <pas/ast/generic/attr_directive.hpp>
bool isArgumentedDirective(const pas::ast::Node &node, const QString name) {
  using namespace pas;
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>() &&
         (node.get<ast::generic::Directive>().value.toUpper() ==
          name.toUpper()) &&
         node.has<ast::generic::Argument>();
}
bool pas::ops::pepp::isBurn::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"BURN"_qs);
}

bool pas::ops::pepp::isEnd::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>() &&
         (node.get<ast::generic::Directive>().value.toUpper() == u"END"_qs);
}

bool pas::ops::pepp::isExport::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"EXPORT"_qs);
}

bool pas::ops::pepp::isImport::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"IMPORT"_qs);
}

bool pas::ops::pepp::isInput::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"INPUT"_qs);
}

bool pas::ops::pepp::isOutput::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"OUTPUT"_qs);
}

bool pas::ops::pepp::isSCall::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"SCALL"_qs);
}

bool pas::ops::pepp::isSection::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>() &&
         (node.get<ast::generic::Directive>().value.toUpper() ==
          u"SECTION"_qs) &&
         node.has<ast::generic::SectionFlags>() &&
         (node.has<ast::generic::ArgumentList>() ||
          node.has<ast::generic::Argument>());
}

bool pas::ops::pepp::isUSCall::operator()(const ast::Node &node) {
  return isArgumentedDirective(node, u"USCALL"_qs);
}
