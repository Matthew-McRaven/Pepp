#include "memmap.hpp"

QList<obj::AddressRegion> obj::getMemoryMap(const ELFIO::elfio &elf) {
  auto ret = QList<obj::AddressRegion>{};
  for (auto &seg : elf.segments) {
    if (seg->get_type() != ELFIO::PT_LOAD)
      continue;
    bool r = seg->get_flags() & ELFIO::PF_R, w = seg->get_flags() & ELFIO::PF_W,
         x = seg->get_flags() & ELFIO::PF_X;
    quint16 minOffset = seg->get_virtual_address();
    quint16 maxOffset =
        minOffset + std::max<quint64>(seg->get_memory_size() - 1, 0);
    ret.push_back(AddressRegion{.r = r,
                                .w = w,
                                .x = x,
                                .minOffset = minOffset,
                                .maxOffset = maxOffset,
                                .seg = &*seg});
  }
  return ret;
}
