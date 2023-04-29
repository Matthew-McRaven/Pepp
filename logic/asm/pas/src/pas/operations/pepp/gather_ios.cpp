#include "./gather_ios.hpp"
#include "pas/ast/generic/attr_directive.hpp"
#include "pas/ast/value/base.hpp"
#include "pas/operations/generic/is.hpp"
#include "pas/operations/pepp/is.hpp"

void pas::ops::pepp::GatherIODefinitions::operator()(const ast::Node &node) {
  if (!node.has<ast::generic::Directive>())
    return;
  auto directive = node.get<ast::generic::Directive>().value;
  if (!(directive == "INPUT" || directive == "OUTPUT"))
    return;
  IO::Direction direction =
      (directive == "INPUT") ? IO::Direction::kInput : IO::Direction::kOutput;
  auto arg = node.get<ast::generic::Argument>().value;
  auto symbol = arg->rawString();
  ios.append({.name = symbol, .direction = direction});
}

QList<pas::ops::pepp::IO>
pas::ops::pepp::gatherIODefinitions(const ast::Node &node) {
  GatherIODefinitions ret;
  generic::Or<pepp::isInput, pepp::isOutput> pred;
  ast::apply_recurse_if(node, pred, ret);
  return ret.ios;
}
