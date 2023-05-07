#include "memmap.hpp"

QList<obj::SegmentRegion> obj::getLoadableSegments(const ELFIO::elfio &elf) {
  auto ret = QList<obj::SegmentRegion>{};
  for (auto &seg : elf.segments) {
    // Treat buffered segments as loadable, otherwise there will be a memory
    // hole where the buffered segments are supposed to go.
    if (!(seg->get_type() == ELFIO::PT_LOAD ||
          seg->get_type() == ELFIO::PT_LOPROC + 1))
      continue;
    bool r = seg->get_flags() & ELFIO::PF_R, w = seg->get_flags() & ELFIO::PF_W,
         x = seg->get_flags() & ELFIO::PF_X;
    quint16 minOffset = seg->get_virtual_address();
    quint16 maxOffset =
        minOffset + std::max<quint64>(seg->get_memory_size() - 1, 0);
    ret.push_back(SegmentRegion{.r = r,
                                .w = w,
                                .x = x,
                                .minOffset = minOffset,
                                .maxOffset = maxOffset,
                                .seg = &*seg});
  }
  return ret;
}

QList<obj::MemoryRegion>
obj::mergeSegmentRegions(QList<SegmentRegion> regions) {
  auto ret = QList<obj::MemoryRegion>{};
  auto similar = [](const MemoryRegion &reg, const SegmentRegion &seg) {
    return reg.r == seg.r && reg.w == seg.w &&
           // Check overlap from either end, PT_LOAD segments are sorted by
           // vaddr, but buffered segments are not.
           (reg.maxOffset + 1 == seg.minOffset ||
            seg.maxOffset + 1 == reg.minOffset);
  };
  for (auto &reg : regions) {
    bool found = false;
    // Attempt to merge the newly visited region into an exising region if
    // possible. Necessary otherwise we end up with too many memory chips and
    // lose perf.
    for (auto &it : ret) {
      if (!similar(it, reg))
        continue;
      it.maxOffset = std::max(it.maxOffset, reg.maxOffset);
      it.minOffset = std::min(it.minOffset, reg.minOffset);
      // Prevent non-loading segments from being auto-loaded at system creation.
      // i.e., user programs.
      if (reg.seg->get_type() == ELFIO::PT_LOAD)
        it.segs.push_back(reg.seg);
      found = true;
    }
    if (!found)
      ret.push_back(MemoryRegion{.r = reg.r,
                                 .w = reg.w,
                                 .minOffset = reg.minOffset,
                                 .maxOffset = reg.maxOffset,
                                 .segs = {reg.seg}});
  }
  return ret;
}
