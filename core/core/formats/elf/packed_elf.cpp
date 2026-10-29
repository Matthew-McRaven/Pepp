/*
 * /Copyright (c) 2026. Stanley Warford, Matthew McRaven
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "packed_elf.hpp"
#include <fstream>

pepp::bts::AElfSource::~AElfSource() = default;

pepp::bts::MappedFileSource::MappedFileSource(std::shared_ptr<MappedFile> file) : _file(std::move(file)) {
  if (!_file) throw std::invalid_argument("MappedFileSource: file must be non-null");
  _size = _file->size();
}

std::shared_ptr<pepp::bts::MappedFileSource> pepp::bts::MappedFileSource::open(const std::filesystem::path &path) {
  std::error_code ec;
  std::filesystem::file_size(path, ec);
  if (ec || !std::ifstream(path, std::ios::binary)) return nullptr;
  return std::make_shared<MappedFileSource>(MappedFile::open_readonly(path.string()));
}

u64 pepp::bts::MappedFileSource::size() const noexcept { return _size; }

std::shared_ptr<pepp::bts::AStorage> pepp::bts::MappedFileSource::slice(u64 offset, u64 length) const {
  // Clamp the requested slice according to the file's size to avoid exceptions.
  const u64 from = std::min<u64>(offset, _size), count = std::min<u64>(length, _size - from);
  // Mapping an empty region fails on every platform we support, so do not ask for one.
  if (count == 0) return std::make_shared<NullStorage>();
  return std::make_shared<MemoryMapped>(_file->slice(from, count));
}

pepp::bts::BufferSource::BufferSource(std::vector<char> &&bytes)
    : _bytes(std::make_shared<BlockStorage>(std::move(bytes))) {}

u64 pepp::bts::BufferSource::size() const noexcept { return _bytes->size(); }

std::shared_ptr<pepp::bts::AStorage> pepp::bts::BufferSource::slice(u64 offset, u64 length) const {
  const u64 total = _bytes->size(), from = std::min<u64>(offset, total);
  const u64 count = std::min<u64>(length, total - from);
  if (count == 0) return std::make_shared<NullStorage>();
  return std::make_shared<BlockStorage::BlockStorageSlice>(_bytes, from, count);
}

pepp::bts::SliceSource::SliceSource(std::shared_ptr<const AElfSource> parent, u64 offset, u64 length)
    : _parent(std::move(parent)), _offset(offset), _length(length) {
  if (!_parent) throw std::invalid_argument("SliceSource: parent must be non-null");
  else if (_offset > _parent->size() || _length > _parent->size() - _offset)
    throw std::invalid_argument("SliceSource exceeds its parent");
}

u64 pepp::bts::SliceSource::size() const noexcept { return _length; }

std::shared_ptr<pepp::bts::AStorage> pepp::bts::SliceSource::slice(u64 offset, u64 length) const {
  const u64 from = std::min<u64>(offset, _length), count = std::min<u64>(length, _length - from);
  if (count == 0) return std::make_shared<NullStorage>();
  return _parent->slice(_offset + from, count);
}
