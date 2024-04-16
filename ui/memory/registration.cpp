#include "registration.hpp"
#include <qqml.h>
#include "./hexdump/memorybytemodel.hpp"

void memory::registerTypes(const char *uri) {
  //  Note, these models are instantiated in C++ and passed to QML. QML
  //  cannot instantiate these models directly
  qmlRegisterUncreatableType<MemoryByteModel>("edu.pepp", 1, 0, "MemByteRoles", "Error: only enums");
}
