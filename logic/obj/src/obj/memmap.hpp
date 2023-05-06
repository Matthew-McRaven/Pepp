#pragma once
#include <QtCore>
#include <elfio/elfio.hpp>
namespace obj {

struct AddressRegion {
  bool r, w, x;
  quint16 minOffset, maxOffset;
  const ELFIO::segment *seg;
  bool operator==(const AddressRegion &other) const = default;
};

QList<AddressRegion> getMemoryMap(const ELFIO::elfio &elf);
} // namespace obj
