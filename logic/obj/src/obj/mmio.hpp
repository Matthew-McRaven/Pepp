#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>

namespace obj {

// May be nullptr if no MMIO note section.
const ELFIO::section *getMMIONoteSection(const ELFIO::elfio &elf);
// If no MMIO note section, will create.
ELFIO::section *addMMIONoteSection(ELFIO::elfio &elf);

struct IO {
  QString name;
  enum class Direction { kInput, kOutput } direction;
};
void addMMIODeclarations(ELFIO::elfio &elf, ELFIO::section *symTab,
                         QList<IO> mmios);
struct AddressedIO : public IO {
  quint16 minOffset, maxOffset;
};

QList<AddressedIO> getMMIODeclarations(const ELFIO::elfio &elf);

void addMMIBuffer(ELFIO::elfio &elf, const ELFIO::segment *bufferableSe);
struct MMIBuffer {
  ELFIO::segment *seg;
  QString portName;
};
QList<MMIBuffer> getMMIBuffers(const ELFIO::elfio &elf);

// Automatically finds the boot flag in the symbol table and adds a note.
void setBootFlagAddress(ELFIO::elfio &elf, QString name = "bootFlg");
std::optional<quint16> getBootFlagsAddress(const ELFIO::elfio &elf);
} // namespace obj
