#include "registration.hpp"
#include <qqml.h>
#include "./aproject.hpp"
#include "utils/strings.hpp"

static const char *error_only_project = "Can only be created through Project::";
void project::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<Pep10_ISA>(uri, 1, 0, "Pep10ISA", error_only_project);
  qmlRegisterType<ProjectModel>(uri, 1, 0, "ProjectModel");
}
