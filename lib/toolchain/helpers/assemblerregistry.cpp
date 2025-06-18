#include "assemblerregistry.hpp"

QSharedPointer<builtins::Registry> helpers::registry_with_assemblers(QString directory) {
  auto registry = QSharedPointer<builtins::Registry>::create(directory);
  return registry;
}
