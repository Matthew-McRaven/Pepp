#pragma once
#include <QtCore>

namespace ELFIO {
class section;
class elfio;
} // namespace ELFIO

namespace pepp::tc {
ELFIO::section *addStrTab(ELFIO::elfio &elf);
}
