#include "dict.hpp"
#include "core/math/bitmanip/enums.hpp"

NiceDictHeader::NiceDictHeader(const Interpreter *interp, u16 nt_addr) : _interp(interp), _nt_addr(nt_addr) {}

std::string_view NiceDictHeader::name() const {
  const auto len = strlen_flags() & (u8)Flags::MAX_LEN;
  const u8 start_offset = _nt_addr - len - 1;
  auto addr = _interp->memory.data() + start_offset;
  return std::string_view((const char *)addr, len + 1);
}

u16 NiceDictHeader::cfa() const { return _nt_addr + (u16)RawDictHeader::StaticOffsets::CODE; }

u16 NiceDictHeader::codeword() { return _interp->read<u16>(cfa()); }

u16 NiceDictHeader::link() const { return _interp->read<u16>(_nt_addr + (u16)RawDictHeader::StaticOffsets::LINK); }

u8 NiceDictHeader::strlen_flags() const {
  return _interp->read<u8>(_nt_addr + (u16)RawDictHeader::StaticOffsets::FLAGS);
}

NiceDictHeader dict_insert(Interpreter *interp, std::string name, Flags flags, std::span<const u16> code,
                           u16 codeword) {
  using namespace bits;
  static const u16 alignment = 2;
  const bool needs_null = name.ends_with("\0");
  auto *here = &interp->cb.here;
  // Add pading before string so that CFA will be aligned. Optionally 1 to enforce that all strings are null terminated.
  const auto unpadded_cfa = *here + (needs_null ? 1 : 0) + (u16)RawDictHeader::StaticOffsets::CODE + name.size();
  const u8 pad = (alignment - (unpadded_cfa % alignment)) % alignment;
  *here = interp->zeros(*here, pad);
  // Write out name
  *here = interp->write(*here, bits::span<const u8>{(const u8 *)name.data(), name.size()});
  // Add null terminator if source does not already include it.
  if (needs_null) *here = interp->zeros(*here, 1);

  // Write out backlink
  const u16 addr_of_link = *here;
  interp->write_here_pp(interp->cb.latest);

  // Compute len and combine with flags, accounting for masks & padding
  const u8 len = name.size() & (u8)Flags::MAX_LEN;
  const u8 with_flags = len | (u8)(flags & Flags::FLAG_MASK);
  interp->write_here_pp(with_flags);
  interp->write_here_pp<u8>(0);
  // Write out codeword
  if (codeword == 0) interp->write_here_pp<u16>(*here + 2);
  else interp->write_here_pp<u16>(codeword);
  // And write out any associated code.
  if (!code.empty()) interp->write_here_pp(code);

  interp->cb.latest = addr_of_link;
  return NiceDictHeader(interp, addr_of_link);
}