#pragma once

#include "common_types.hpp"
#include "core/integers.h"

// Effectively a std::map wrapper that exists so that blueprints (which cannot use Qt) can refer to files opaquely.
// I don't want to leak Mipmaps into blueprints because they are purely visual objects.
class FileStore {
public:
  using Key = schematic::ImageFileKey;

  // Insert or replace an entry. Returns the key (for chaining or caller convenience).
  Key insert(const std::string &source);

  //  Returns nullptr if absent.
  std::optional<const std::string *> find(Key key) const;
  std::optional<Key> find(const std::string &source) const;

  bool contains(Key key) const;                   // Is the key present?
  bool contains(const std::string &source) const; // Is the source present?
  std::size_t size() const;
  const std::unordered_map<Key, std::string> &entries() const;

private:
  std::unordered_map<Key, std::string> _entries;
  std::unordered_map<std::string, Key> _inverse;
  u32 _next_key = 1;
};
