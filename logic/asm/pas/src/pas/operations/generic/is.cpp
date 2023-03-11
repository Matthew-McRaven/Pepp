#include "./is.hpp"
#include "pas/ast/generic/attr_argument.hpp"
#include "pas/ast/generic/attr_comment.hpp"
#include "pas/ast/generic/attr_comment_indent.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/generic/attr_macro.hpp"
#include "pas/ast/node.hpp"

bool pas::ops::generic::isBlank::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Blank;
}

bool pas::ops::generic::isComment::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value == ast::generic::Type::Comment &&
         node.has<ast::generic::Comment>() &&
         node.has<ast::generic::CommentIndent>();
}

bool pas::ops::generic::isAlign::operator()(const ast::Node &node) {
  (void)(void *)(&node);
  auto n = node.get<ast::generic::Directive>().value;
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>() &&
         node.get<ast::generic::Directive>().value.toUpper() == u"ALIGN"_qs &&
         node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isString::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>() &&
         directiveAliases.contains(
             node.get<ast::generic::Directive>().value.toUpper()) &&
         node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isSkip::operator()(const ast::Node &node) {
  if (allowFill)
    return node.get<ast::generic::Type>().value ==
               ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(
               node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::ArgumentList>();
  else
    return node.get<ast::generic::Type>().value ==
               ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(
               node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isByte1::operator()(const ast::Node &node) {
  if (allowMultiple)
    return node.get<ast::generic::Type>().value ==
               ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(
               node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::ArgumentList>();
  else
    return node.get<ast::generic::Type>().value ==
               ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(
               node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isByte2::operator()(const ast::Node &node) {
  if (allowMultiple)
    return node.get<ast::generic::Type>().value ==
               ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(
               node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::ArgumentList>();
  else
    return node.get<ast::generic::Type>().value ==
               ast::generic::Type::Directive &&
           node.has<ast::generic::Directive>() &&
           directiveAliases.contains(
               node.get<ast::generic::Directive>().value.toUpper()) &&
           node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isSet::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>() &&
         directiveAliases.contains(
             node.get<ast::generic::Directive>().value.toUpper()) &&
         node.has<ast::generic::Argument>();
}

bool pas::ops::generic::isDirective::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::Directive &&
         node.has<ast::generic::Directive>();
}

bool pas::ops::generic::isMacro::operator()(const ast::Node &node) {
  return node.get<ast::generic::Type>().value ==
             ast::generic::Type::MacroInvoke &&
         node.has<ast::generic::Macro>();
}
