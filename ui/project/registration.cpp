#include "registration.hpp"
#include <qqml.h>
#include "./aproject.hpp"
#include "utils/strings.hpp"

static const char *error_only_project = "Can only be created through Project::";
void project::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<DebugEnableFlags>(uri, 1, 0, "DebugEnableFlags", utils::error_only_enums);
  qmlRegisterUncreatableType<StepEnableFlags>(uri, 1, 0, "StepEnableFlags", utils::error_only_enums);
  qmlRegisterUncreatableType<Pep10_ISA>(uri, 1, 0, "Pep10ISA", error_only_project);
  qmlRegisterType<ProjectModel>(uri, 1, 0, "ProjectModel");
}
