#include "filestore.hpp"

FileStore::Key FileStore::insert(const std::string &source) {
  if (auto it = _inverse.find(source); it != _inverse.end()) {
    return it->second;
  }

  schematic::ImageFileKey key{_next_key++};
  _inverse.emplace(source, key);
  _entries.emplace(key, source);
  return key;
}

std::optional<const std::string *> FileStore::find(Key key) const {
  return _entries.contains(key) ? std::optional<const std::string *>(&_entries.at(key)) : std::nullopt;
}

std::optional<FileStore::Key> FileStore::find(const std::string &source) const {
  return _inverse.contains(source) ? std::optional<Key>(_inverse.at(std::string(source))) : std::nullopt;
}

bool FileStore::contains(Key key) const { return _entries.contains(key); }

bool FileStore::contains(const std::string &source) const { return _inverse.contains(source); }

std::size_t FileStore::size() const { return _entries.size(); }

const std::unordered_map<FileStore::Key, std::string> &FileStore::entries() const { return _entries; }
