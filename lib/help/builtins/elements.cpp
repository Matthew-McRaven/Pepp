#include "elements.hpp"

QString builtins::Element::contents() const {
  if (!_contents.has_value()) _contents = contentsFn();
  return *_contents;
}
