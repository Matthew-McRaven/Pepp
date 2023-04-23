#include "./suppress_object.hpp"
#include "pas/ast/generic/attr_hide.hpp"

void pas::ops::generic::SuppressObject::operator()(ast::Node &node) {
  ast::generic::Hide hide;
  if (node.has<ast::generic::Hide>())
    hide = node.get<ast::generic::Hide>();
  hide.value.object = ast::generic::Hide::In::Object::NoEmit_CountSize;
  node.set(hide);
}
