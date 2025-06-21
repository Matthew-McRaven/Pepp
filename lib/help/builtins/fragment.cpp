#include "fragment.hpp"

QString builtins::Fragment::contents() const {
  if (!_contents.has_value()) _contents = contentsFn();
  return *_contents;
}
