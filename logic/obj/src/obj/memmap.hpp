#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>
namespace obj {

struct SegmentRegion {
  bool r, w, x;
  quint16 minOffset, maxOffset;
  const ELFIO::segment *seg;
  bool operator==(const SegmentRegion &other) const = default;
};

// Each loadable segment will have exactly 1 SegmentRegion returned
QList<SegmentRegion> getLoadableSegments(const ELFIO::elfio &elf);

struct MemoryRegion {
  bool r, w;
  quint16 minOffset, maxOffset;
  QList<const ELFIO::segment *> segs;
};
// Merge "similar" SegmentRegion's into a single effective unit.
// Reduces the number of unique memory targets needed to implement system.
// Always ignores X (for now).
QList<MemoryRegion> mergeSegmentRegions(QList<SegmentRegion> regions);
} // namespace obj
