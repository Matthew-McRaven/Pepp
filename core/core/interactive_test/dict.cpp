#include "dict.hpp"
#include "core/math/bitmanip/enums.hpp"

NiceDictHeader::NiceDictHeader(Interpreter *interp, u16 nt_addr) : _interp(interp), _nt_addr(nt_addr) {}

std::string_view NiceDictHeader::name() const {
  const auto len = strlen_flags() & (u8)Flags::MAX_LEN;
  const u16 start_offset = _nt_addr - len - 1;
  auto addr = _interp->memory.data() + start_offset;
  return std::string_view((const char *)addr, len);
}

u16 NiceDictHeader::cfa() const { return _nt_addr + (u16)RawDictHeader::StaticOffsets::CODE; }

u16 NiceDictHeader::dfa() const { return _nt_addr + (u16)RawDictHeader::StaticOffsets::DATA; }

u16 NiceDictHeader::codeword() { return _interp->read<u16>(cfa()); }

u16 NiceDictHeader::link() const { return _interp->read<u16>(link_addr()); }

u16 NiceDictHeader::link_addr() const { return _nt_addr + (u16)RawDictHeader::StaticOffsets::LINK; }

u8 NiceDictHeader::strlen_flags() const {
  return _interp->read<u8>(_nt_addr + (u16)RawDictHeader::StaticOffsets::FLAGS);
}

void NiceDictHeader::toggle_hidden() {
  u8 flags = strlen_flags();
  flags ^= (u8)Flags::HIDDEN;
  _interp->write(flags, _nt_addr + (u16)RawDictHeader::StaticOffsets::FLAGS);
}

NiceDictHeader dict_insert(Interpreter *interp, std::string_view name, Flags flags, std::span<const u16> code,
                           u16 codeword) {
  auto ret = dict_header(interp, name, flags);
  auto *here = &interp->cb.here;
  // Write out codeword
  if (codeword == 0) interp->write_here_pp<u16>(*here + 2);
  else interp->write_here_pp<u16>(codeword);
  interp->cb.latest = ret.link_addr();
  // And write out any associated code.
  if (!code.empty()) interp->write_here_pp(code);
  return ret;
}

NiceDictHeader dict_header(Interpreter *interp, std::string_view name, Flags flags) {
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

  return NiceDictHeader(interp, addr_of_link);
}
