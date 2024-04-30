#include "registration.hpp"
#include <qqml.h>
#include "registermodel.hpp"

void cpu::registerTypes(const char *uri) { qmlRegisterType<RegisterModel>(uri, 1, 0, "RegisterModel"); }
