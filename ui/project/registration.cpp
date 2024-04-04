#include "registration.hpp"
#include <qqml.h>
#include "./aproject.hpp"
#include "utils/strings.hpp"

static const char *error_only_project = "Can only be created through Project::";
void project::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<ISAProject>(uri, 1, 0, "ISAProject", error_only_project);
  qmlRegisterSingletonType<Projects>(uri, 1, 0, "Projects",
                                     [](QQmlEngine *, QJSEngine *) -> QObject * { return new Projects(); });
}
