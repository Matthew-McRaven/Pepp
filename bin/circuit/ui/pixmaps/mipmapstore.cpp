#include "mipmapstore.hpp"
#include "schematic/circuitproject.hpp"

MipmapStore::MipmapStore(std::shared_ptr<CircuitProject> project) : _project(std::move(project)) {}

MipmapStore::Key MipmapStore::insert(MipmapSource source, QSize base_size, Direction dir,
                                     MipmapConstraint constraints) {
  MipmapEntry entry;
  const Key key{(u32)_next_key++};
  if (!source.source_path().isEmpty()) {
    const auto file_path = source.source_path().toStdString();
    _project->track_file(file_path);
    _source_to_key[file_path] = key;
  }

  entry.source = std::move(source);
  entry.mipmap = entry.source.build(base_size, dir, constraints);
  _entries[key] = std::move(entry);
  return key;
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

std::optional<MipmapStore::Key> MipmapStore::find(const std::string &file_path) const {
  auto it = _source_to_key.find(file_path);
  return it != _source_to_key.cend() ? std::optional<Key>{it->second} : std::nullopt;
}

const MipmappedPrerotatedPixmap *MipmapStore::mipmap(Key key) const {
  if (!_entries.contains(key)) throw std::out_of_range("MipmapStore::mipmap: key not found");
  return &_entries.at(key).mipmap;
}

bool MipmapStore::contains(Key key) const { return _entries.find(key) != _entries.end(); }

std::size_t MipmapStore::size() const { return _entries.size(); }

void MipmapStore::debug_dump_to_dir(QString dir) const {
  for (const auto &[key, entry] : _entries) {
    const auto base_path = dir + "/" + QString::number(key.value);
    for (int level = 0; level < entry.mipmap.level_count(); ++level) {
      auto at_level = entry.mipmap.at_level(level);
      at_level->down()->save(base_path + "_mip" + QString::number(level) + "_down.png");
      at_level->left()->save(base_path + "_mip" + QString::number(level) + "_left.png");
      at_level->right()->save(base_path + "_mip" + QString::number(level) + "_right.png");
      at_level->up()->save(base_path + "_mip" + QString::number(level) + "_up.png");
    }
  }
}

const std::unordered_map<MipmapStore::Key, MipmapEntry> &MipmapStore::entries() const { return _entries; }
