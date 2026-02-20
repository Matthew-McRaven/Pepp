#include "addr2line.hpp"
#include "toolchain/pas/ast/generic/attr_address.hpp"
#include "toolchain/pas/ast/generic/attr_location.hpp"
#include "toolchain/pas/ast/node.hpp"

void pas::ops::generic::addr2line::operator()(const ast::Node &node) {
  if (node.has<ast::generic::Address>()) {
    auto address = node.get<ast::generic::Address>().value;
    if (useList && node.has<ast::generic::ListingLocation>()) {
      auto loc = node.get<ast::generic::ListingLocation>().value;
      if (loc.valid) return;
      mapping.emplace_back(std::pair<int, quint32>(loc.line, address.start));
    } else if (node.has<ast::generic::RootLocation>()) {
      auto loc = node.get<ast::generic::RootLocation>().value;
      if (loc.valid) return;
      mapping.emplace_back(std::pair<int, quint32>(loc.line, address.start));
    }
  }
}

std::vector<std::pair<int, quint32>> pas::ops::generic::source2addr(const ast::Node &node) {
  addr2line lines;
  lines.useList = false;
  pas::ast::apply_recurse(node, lines);
  return lines.mapping;
}

std::vector<std::pair<int, quint32>> pas::ops::generic::list2addr(const ast::Node &node) {
  addr2line lines;
  lines.useList = true;
  pas::ast::apply_recurse(node, lines);
  return lines.mapping;
}
