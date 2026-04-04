#include "mipmapstore.hpp"

MipmapStore::Key MipmapStore::insert(MipmapSource source, QSize base_size, Direction dir,
                                     MipmapConstraint constraints) {
  MipmapEntry entry;
  entry.source = std::move(source);
  entry.mipmap = entry.source.build(base_size, dir, constraints);
  const Key key = _next_key++;
  _entries[key] = std::move(entry);
  return key;
}

bool MipmapStore::replace(Key key, MipmapSource source, QSize base_size, Direction dir, MipmapConstraint constraints) {
  if (!_entries.contains(key)) {
    return false;
  }
  MipmapEntry entry;
  entry.source = std::move(source);
  entry.mipmap = entry.source.build(base_size, dir, constraints);
  _entries[key] = std::move(entry);
  return true;
}

void MipmapStore::recalculate(Key key, QSize base_size, Direction dir, MipmapConstraint constraints) {
  if (auto entry = find(key)) {
    entry->mipmap = entry->source.build(base_size, dir, constraints);
  }
}

const MipmapEntry *MipmapStore::find(Key key) const {
  auto it = _entries.find(key);
  return it != _entries.cend() ? &it->second : nullptr;
}

MipmapEntry *MipmapStore::find(Key key) {
  auto it = _entries.find(key);
  return it != _entries.cend() ? &it->second : nullptr;
}

const MipmappedPrerotatedPixmap &MipmapStore::mipmap(Key key) const {
  if (!_entries.contains(key)) {
    throw std::out_of_range("MipmapStore::mipmap: key not found");
  }
  return _entries.at(key).mipmap;
}

bool MipmapStore::contains(Key key) const { return _entries.find(key) != _entries.end(); }

std::size_t MipmapStore::size() const { return _entries.size(); }

const std::unordered_map<MipmapStore::Key, MipmapEntry> &MipmapStore::entries() const { return _entries; }
