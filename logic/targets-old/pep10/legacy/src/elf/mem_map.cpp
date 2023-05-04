#include "mem_map.hpp"

result<std::vector<elf_tools::pep10::PortDefinition>> elf_tools::pep10::port_definitions(const ELFIO::elfio &image) {
  // Fetch symtab so we can resolve masm::elf::mmio::Definition::symbol_table_offset to a string.
  auto symtab = elf_tools::find_section(image, "os.symtab");
  if (symtab == nullptr)
    return status_code(PepElfErrc::NoOSSymtab);
  const ELFIO::symbol_section_accessor symbols(image, symtab);

  // Fetch mmio section, and run it through our custom decoder to get a vector of masm::elf::mmio::Definition.
  auto mmio = elf_tools::find_section(image, "os.mmio");
  if (mmio == nullptr)
    return status_code(PepElfErrc::NoOSMMIO);
  auto bytes = std::vector<uint8_t>{mmio->get_data(), mmio->get_data() + mmio->get_size()};
  // Will fail if bytes%4 != 0.
  auto ports_result = masm::elf::mmio::from_bytes(bytes);
  if (ports_result.has_error())
    return ports_result.error().clone();
  auto ports = ports_result.value();

  // Don't pass size to ctor, or there will be a bunch of blank entires in vector.
  auto ret_val = std::vector<elf_tools::pep10::PortDefinition>();
  ret_val.reserve(ports.size());

  // Fields needed to perform ELFIO symbol lookup.
  // I will ignore most of them, but the function call will fail to compile without them.
  std::string name;
  ELFIO::Elf64_Addr value;
  ELFIO::Elf_Xword size;
  unsigned char bind, type, other;
  ELFIO::Elf_Half section_index;

  for (auto port : ports) {
    // It is UB if I attempt to index beyond the range of the symtab, therefore perform a length check.
    // offset is 0-index, symbols_num is 1-indexed.
    if (port.symbol_table_offset > symbols.get_symbols_num() - 1)
      return status_code(PepElfErrc::BadMMIOSymbol);
    symbols.get_symbol(port.symbol_table_offset, name, value, size, bind, type, section_index, other);
    ret_val.emplace_back(elf_tools::pep10::PortDefinition{static_cast<uint16_t>(value), port.type, name});
  }
  return ret_val;

}
