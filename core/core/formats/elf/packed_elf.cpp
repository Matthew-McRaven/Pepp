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

pepp::bts::AElfSource::~AElfSource() = default;

pepp::bts::MappedFileSource::MappedFileSource(std::shared_ptr<MappedFile> file) : _file(std::move(file)) {
  if (!_file) throw std::invalid_argument("MappedFileSource: file must be non-null");
}

std::shared_ptr<pepp::bts::AStorage> pepp::bts::MappedFileSource::slice(u64 offset, u64 length) const {
  // Mapping an empty region fails on every platform we support, so do not ask for one.
  if (length == 0) return std::make_shared<NullStorage>();
  return std::make_shared<MemoryMapped>(_file->slice(offset, length));
}

pepp::bts::BufferSource::BufferSource(std::vector<char> &&bytes)
    : _bytes(std::make_shared<BlockStorage>(std::move(bytes))) {}

std::shared_ptr<pepp::bts::AStorage> pepp::bts::BufferSource::slice(u64 offset, u64 length) const {
  const u64 total = _bytes->size(), from = std::min<u64>(offset, total);
  const u64 count = std::min<u64>(length, total - from);
  if (count == 0) return std::make_shared<NullStorage>();
  return std::make_shared<BlockStorage::BlockStorageSlice>(_bytes, from, count);
}
