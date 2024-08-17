#include "registration.hpp"
#include <qqml.h>
#include "preferencemodel.hpp"

void prefs::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<PreferenceModel>("edu.pepperdine", 1, 0, "PrefProperty", "Error: only enums");
}
