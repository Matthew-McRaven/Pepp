#include "registration.hpp"
#include <qqml.h>
#include "./utils.hpp"

void constants::registerTypes(const char *uri) {
  qmlRegisterUncreatableType<constants::Abstraction>(uri, 1, 0, "Abstraction", error_only_enums);
  qmlRegisterUncreatableType<constants::Architecture>(uri, 1, 0, "Architecture", error_only_enums);
}
