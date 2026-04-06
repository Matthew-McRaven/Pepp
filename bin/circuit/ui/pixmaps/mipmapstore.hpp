#pragma once

#include "common_types.hpp"
#include "core/integers.h"
#include "mipmapsource.hpp"

// An entry in the store is the source + the mipmap built from it.
struct MipmapEntry {
  MipmapSource source;
  MipmappedPrerotatedPixmap mipmap;
};

class CircuitProject;

// Effectively a wrapper for std::map, but with specializations on insert/replace that generate mipmaps on our behalf.
// They Key is opaque and is entirely unrelated to DiagramType::Type. The key 0 is reserved to indicate an invalid key.
// Once inserted, keys cannot be deleted to prevent use-after frees. The contents of an entry can be replaced.
// Replacing contents might occur when light mode is switched to dark.
// recalculate() exists to rebuild mips level when display properties change.
class MipmapStore {
public:
  MipmapStore(std::shared_ptr<CircuitProject> project);
  using Key = schematic::MipmapStoreKey;

  // Returns the key (for chaining or caller convenience).
  Key insert(MipmapSource source, QSize base_size, Direction dir, MipmapConstraint constraints = {});
  void recalculate(Key key, QSize base_size, Direction dir, MipmapConstraint constraints = {});

  //  Returns nullptr if absent.
  const MipmapEntry *find(Key key) const;
  MipmapEntry *find(Key key);
  std::optional<Key> find(const std::string &file_path) const;

  // Access mipmap directly and throws if absent.
  const MipmappedPrerotatedPixmap &mipmap(Key key) const;
  bool contains(Key key) const; // Is the key present?
  std::size_t size() const;
  const std::unordered_map<Key, MipmapEntry> &entries() const;

private:
  int _next_key = 1;
  std::unordered_map<Key, MipmapEntry> _entries;
  std::unordered_map<std::string, Key> _source_to_key;
  std::shared_ptr<CircuitProject> _project;
};
