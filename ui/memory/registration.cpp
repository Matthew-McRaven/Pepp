#include "registration.hpp"
#include <qqml.h>
#include "./hexdump/memorybytemodel.hpp"

void memory::registerTypes(const char *uri) {
  //  Note, these models are instantiated in C++ and passed to QML. QML
  //  cannot instantiate these models directly
  qmlRegisterType<MemoryByteModel>("edu.pepp", 1, 0, "MemoryModel");
  qmlRegisterUncreatableType<MemoryRoles>("edu.pepp", 1, 0, "MemoryRoles", "Error: only enums");
  qmlRegisterUncreatableType<MemoryHighlight>("edu.pepp", 1, 0, "MemoryHighlight", "Error: only enums");
  qmlRegisterUncreatableType<EmptyRawMemory>("edu.pepp", 1, 0, "EmptyRawMemory", "Must use create(int)");
  qmlRegisterSingletonType<EmptyRawMemoryFactory>("edu.pepp", 1, 0, "EmptyRawMemoryFactory",
                                                  EmptyRawMemoryFactory::singletonProvider);
  qmlRegisterUncreatableType<ArrayRawMemory>("edu.pepp", 1, 0, "ArrayRawMemory", "Must use create(int)");
  qmlRegisterSingletonType<ArrayRawMemoryFactory>("edu.pepp", 1, 0, "ArrayRawMemoryFactory",
                                                  ArrayRawMemoryFactory::singletonProvider);
}
