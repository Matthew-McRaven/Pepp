#include "./errors.hpp"
#include "pas/ast/node.hpp"
void pas::ops::generic::collectErrors::operator()(const ast::Node &node) {
  using Location = ast::generic::SourceLocation;
  using Message = ast::generic::Message;
  using Error = ast::generic::Error;
  if (node.has<Error>()) {
    Location location;
    if (node.has<Location>())
      location = node.get<Location>();
    for (auto error : node.get<Error>().value) {
      errors.push_back(QPair<Location, Message>{location, error});
    }
  }
}
