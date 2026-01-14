#include "./packed_types.hpp"

void pepp::bts::detail::fill_e_ident(std::span<u8, 16> e_ident, ElfBits B, ElfEndian E, ElfABI abi,
                                     u8 abi_version) noexcept {
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG0)] = to_underlying(ElfMagic::ELFMAG0);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG1)] = to_underlying(ElfMagic::ELFMAG1);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG2)] = to_underlying(ElfMagic::ELFMAG2);
  e_ident[to_underlying(ElfIdentifierIndices::EI_MAG3)] = to_underlying(ElfMagic::ELFMAG3);
  e_ident[to_underlying(ElfIdentifierIndices::EI_CLASS)] =
      to_underlying(B == ElfBits::b64 ? ElfClass::ELFCLASS64 : ElfClass::ELFCLASS32);
  e_ident[to_underlying(ElfIdentifierIndices::EI_DATA)] =
      to_underlying(E == ElfEndian::le ? ElfEncoding::ELFDATA2LSB : ElfEncoding::ELFDATA2MSB);
  e_ident[to_underlying(ElfIdentifierIndices::EI_VERSION)] = to_underlying(ElfVersion::EV_CURRENT);
  e_ident[to_underlying(ElfIdentifierIndices::EI_OSABI)] = to_underlying(abi);
  e_ident[to_underlying(ElfIdentifierIndices::EI_ABIVERSION)] = abi_version;
  auto padding = e_ident.subspan(to_underlying(ElfIdentifierIndices::EI_PAD));
  memset(padding.data(), 0, padding.size());
}
