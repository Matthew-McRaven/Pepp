#include "./mmio.hpp"
#include "bits/operations/copy.hpp"
const ELFIO::section *obj::getMMIONoteSection(const ELFIO::elfio &elf) {
  for (auto &sec : elf.sections)
    if (sec->get_type() == ELFIO::SHT_NOTE)
      return &*sec;
  return nullptr;
}

ELFIO::section *obj::addMMIONoteSection(ELFIO::elfio &elf) {
  // Only create note sec & segment if they do not already exist.
  ELFIO::section *noteSec = nullptr;
  for (auto &sec : elf.sections) {
    if (sec->get_type() == ELFIO::SHT_NOTE)
      noteSec = &*sec;
  }
  if (noteSec == nullptr) {
    noteSec = elf.sections.add(".mmio");
    noteSec->set_type(ELFIO::SHT_NOTE);
  }

  return noteSec;
}

void addNoteSeg(ELFIO::elfio &elf) {
  auto noteSec = obj::addMMIONoteSection(elf);
  ELFIO::segment *noteSeg = nullptr;
  for (auto &seg : elf.segments) {
    if (seg->get_type() == ELFIO::PT_NOTE)
      noteSeg = &*seg;
  }
  if (noteSeg == nullptr) {
    if (noteSeg == nullptr) {
      noteSeg = elf.segments.add();
      noteSeg->set_type(ELFIO::PT_NOTE);
      noteSeg->add_section(noteSec, 1);
    }
  }
}

void obj::addMMIODeclarations(ELFIO::elfio &elf, ELFIO::section *symTab,
                              QList<IO> mmios) {
  ELFIO::symbol_section_accessor symTabAc(elf, symTab);
  auto noteSec = addMMIONoteSection(elf);
  addNoteSeg(elf);
  auto noteAc = ELFIO::note_section_accessor(elf, noteSec);
  // Iterate over the symtab first, since it is much longer than MMIO list
  for (ELFIO::Elf_Xword it = 1; it < symTabAc.get_symbols_num(); it++) {
    std::string name;
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind, type, other;
    ELFIO::Elf_Half index;
    // Skip to next iteration if it does not name a valid symbol
    if (!symTabAc.get_symbol(it, name, value, size, bind, type, index, other))
      continue;

    // Search the MMIO list for a matching entry, skip to next iteration if no
    // match.
    QString nameQs = QString::fromStdString(name);
    auto target = std::find_if(
        mmios.cbegin(), mmios.cend(),
        [&nameQs](const ::obj::IO &io) { return io.name == nameQs; });
    if (target == mmios.cend())
      continue;

    // Must use copy helper to maintain stable bit order between host
    // platforms.
    quint8 desc[2 + 4]; // ELF_half (16b for symtab section index) and
                        // ELF32_WORD (32b for symbol index)
    auto stIndex = symTab->get_index();
    bits::memcpy_endian(desc + 0, bits::Order::BigEndian, 2, stIndex);
    bits::memcpy_endian(desc + 2, bits::Order::BigEndian, 4, it);
    noteAc.add_note(target->direction == ::obj::IO::Direction::kInput ? 0x11
                                                                      : 0x12,
                    "pepp.mmios", (char *)desc, sizeof(desc));
  }
}

QList<obj::AddressedIO> obj::getMMIODeclarations(const ELFIO::elfio &elf) {
  auto noteSec = getMMIONoteSection(elf);
  if (noteSec == nullptr)
    return {};
  auto noteAc = ELFIO::const_note_section_accessor(elf, noteSec);
  auto ret = QList<obj::AddressedIO>{};
  ELFIO::Elf_Word noteType = 0, descSize = 0;
  std::string name;
  char *desc = 0;
  for (int it = 0; it < noteAc.get_notes_num(); it++) {
    // Check that note exists and is from me.
    if (!noteAc.get_note(it, noteType, name, desc, descSize))
      continue;
    else if (name != "pepp.mmios")
      continue;

    // Copy out symbol table index + symbol index into that table.
    auto stIndex = bits::memcpy_endian<ELFIO::Elf_Half>(
        desc + 0, bits::Order::BigEndian, 2);
    auto symIt = bits::memcpy_endian<ELFIO::Elf_Xword>(
        desc + 2, bits::Order::BigEndian, 4);
    auto symTab = elf.sections[stIndex];
    auto symTabAc = ELFIO::symbol_section_accessor(elf, symTab);
    // If symbol exists, extract contents.
    ELFIO::Elf64_Addr value;
    ELFIO::Elf_Xword size;
    unsigned char bind, symbolType, other;
    ELFIO::Elf_Half index;
    // Skip to next iteration if it does not name a valid symbol.
    if (!symTabAc.get_symbol(symIt, name, value, size, bind, symbolType, index,
                             other))
      continue;
    ret.push_back(obj::AddressedIO{
        {.name = QString::fromStdString(name),
         .direction = (noteType == 0x11) ? IO::Direction::kInput
                                         : IO::Direction::kOutput},
        static_cast<quint16>(value),
        static_cast<quint16>(value + std::max<typeof(size)>(size - 1, 0))});
  }
  return ret;
}

void obj::addMMIBuffer(ELFIO::elfio &elf, const ELFIO::segment *bufferableSeg) {
  auto noteSec = addMMIONoteSection(elf);
  addNoteSeg(elf);
  auto noteAc = ELFIO::note_section_accessor(elf, noteSec);
  // Leave two bytes for section index.
  quint8 desc[] = "  diskIn";
  // Must use copy helper to maintain stable bit order between hosts.
  bits::memcpy_endian(desc + 0, bits::Order::BigEndian, 2,
                      bufferableSeg->get_index());
  // No longer skip null character so that it is easy to turn into string on
  // simulator side.
  noteAc.add_note(0x10, "pepp.mmios", (char *)desc, sizeof(desc));
}

QList<obj::MMIBuffer> obj::getMMIBuffers(const ELFIO::elfio &elf) {
  auto noteSec = getMMIONoteSection(elf);
  if (noteSec == nullptr)
    return {};
  auto noteAc = ELFIO::const_note_section_accessor(elf, noteSec);
  auto ret = QList<obj::MMIBuffer>{};
  ELFIO::Elf_Word type, descSize;
  std::string name;
  char *desc;
  for (int it = 0; it < noteAc.get_notes_num(); it++) {
    // Check that note exists and is from me.
    if (!noteAc.get_note(it, type, name, desc, descSize))
      continue;
    else if (name != "pepp.mmios")
      continue;
    else if (type != 0x10)
      continue;

    char *port = desc + 2;
    // Must use copy helper to maintain stable bit order between hosts.
    auto segIdx = bits::memcpy_endian<ELFIO::Elf_Half>(
        desc + 0, bits::Order::BigEndian, 2);
    if (elf.segments.size() - 1 < segIdx)
      continue;
    ret.push_back(
        obj::MMIBuffer{.seg = elf.segments[segIdx], .portName = port});
  }
  return ret;
}

static const quint8 bootFlagAddrSize = 2;
static const quint8 bootFlagSizeSize = 2;
void obj::setBootFlagAddress(ELFIO::elfio &elf, QString name) {
  auto nameStd = name.toStdString();
  // Temporaries to store symbols.
  std::string _name;
  ELFIO::Elf64_Addr value;
  ELFIO::Elf_Xword size;
  unsigned char bind, symbolType, other;
  ELFIO::Elf_Half index;
  for (int i = 0; i < elf.sections.size(); i++) {
    auto sec = elf.sections[i];
    if (sec->get_type() != ELFIO::SHT_SYMTAB)
      continue;
    auto symTabAc = ELFIO::const_symbol_section_accessor(elf, sec);
    for (int j = 0; j < symTabAc.get_symbols_num(); j++) {
      // Skip to next iteration if it does not name a valid symbol.
      if (!symTabAc.get_symbol(j, _name, value, size, bind, symbolType, index,
                               other))
        continue;
      if (nameStd == _name)
        goto found;
    }
  }
  return;
found:
  auto noteSec = addMMIONoteSection(elf);
  auto noteAc = ELFIO::note_section_accessor(elf, noteSec);
  quint8 desc[bootFlagAddrSize + bootFlagSizeSize];
  bits::memcpy_endian(desc, bits::Order::BigEndian, bootFlagAddrSize, &value,
                      bits::hostOrder(), bootFlagAddrSize);
  bits::memcpy_endian(desc + bootFlagAddrSize, bits::Order::BigEndian,
                      bootFlagSizeSize, &size, bits::hostOrder(),
                      bootFlagSizeSize);
  noteAc.add_note(0x10, "pepp.boot", (char *)desc, sizeof(desc));
  ;
}

std::optional<quint16> obj::getBootFlagsAddress(const ELFIO::elfio &elf) {
  auto noteSec = getMMIONoteSection(elf);
  if (noteSec == nullptr)
    return {};
  auto noteAc = ELFIO::const_note_section_accessor(elf, noteSec);
  std::optional<quint16> ret = std::nullopt;

  ELFIO::Elf_Word noteType = 0, descSize = 0;
  std::string name;
  char *desc;
  for (int it = 0; it < noteAc.get_notes_num(); it++) {
    // Check that note exists and is from me.
    if (!noteAc.get_note(it, noteType, name, desc, descSize))
      continue;
    else if (name != "pepp.boot")
      continue;

    auto addr = bits::memcpy_endian<ELFIO::Elf64_Addr>(
        desc, bits::Order::BigEndian,
        std::min<quint64>(descSize, bootFlagAddrSize));
    auto size = bits::memcpy_endian<ELFIO::Elf64_Addr>(
        desc + bootFlagAddrSize, bits::Order::BigEndian,
        std::min<quint64>(descSize - bootFlagAddrSize, bootFlagSizeSize));
    quint64 tmp = 0;
    bits::memcpy_endian(&tmp, bits::hostOrder(), size, &addr, bits::hostOrder(),
                        sizeof(addr));
    ret = tmp;
    break;
  }
  return ret;
}
