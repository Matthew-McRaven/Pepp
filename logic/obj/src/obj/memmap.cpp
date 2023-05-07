#include "memmap.hpp"

QList<obj::SegmentRegion> obj::getLoadableSegments(const ELFIO::elfio &elf) {
  auto ret = QList<obj::SegmentRegion>{};
  for (auto &seg : elf.segments) {
    if (seg->get_type() != ELFIO::PT_LOAD)
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
           reg.maxOffset + 1 == seg.minOffset;
  };
  for (auto &reg : regions) {
    if (ret.size() > 0 && similar(ret.back(), reg)) {
      auto &back = ret.back();
      back.maxOffset = reg.maxOffset;
      back.segs.push_back(reg.seg);
    } else {
      ret.push_back(MemoryRegion{.r = reg.r,
                                 .w = reg.w,
                                 .minOffset = reg.minOffset,
                                 .maxOffset = reg.maxOffset,
                                 .segs = {reg.seg}

      });
    }
  }
  return ret;
}
