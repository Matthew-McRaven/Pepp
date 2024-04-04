#include "aproject.hpp"

QString ISAProject::objectCodeText() const { return _objectCodeText; }

void ISAProject::setObjectCodeText(const QString &objectCodeText) {
  if (_objectCodeText == objectCodeText)
    return;
  _objectCodeText = objectCodeText;
  emit objectCodeTextChanged();
}

ISAProject *Projects::isa(utils::Architecture::Value arch, project::Features features = project::Features::None) {
  return new ISAProject(nullptr, features);
}
