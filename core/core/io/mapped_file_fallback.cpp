/*
 * Copyright (c) 2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "core/io/mapped_file.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace {
// Return all bytes of a file or nullopt if not readable.
std::optional<std::vector<u8>> read_all(const std::string &path) {
  std::error_code error;
  const auto size = std::filesystem::file_size(path, error);
  if (error) return std::nullopt;
  std::ifstream file(path, std::ios::binary);
  std::vector<u8> ret(size);
  if (!file || !file.read(reinterpret_cast<char *>(ret.data()), static_cast<std::streamsize>(size)))
    return std::nullopt;
  return ret;
}
} // namespace

class pepp::bts::BufferedFile::BufferedSlice final : public MappedFile::Slice {
public:
  BufferedSlice(std::shared_ptr<BufferedFile> file, size_t offset, size_t length)
      : _file(std::move(file)), _offset(offset), _length(length) {}
  size_t size() const noexcept override { return _length; }
  bool readonly() const noexcept override { return _file->_readonly; }
  bits::span<u8> get() noexcept override { return view(); }
  bits::span<const u8> get() const noexcept override { return view(); }
  void flush() override {
    if (!_file->_readonly) _file->write_back(std::min(_offset, _file->_data.size()), view().size());
  }

private:
  // Returns the slice clipped to the buffer, which holds the whole file.
  bits::span<u8> view() const noexcept {
    auto &data = _file->_data;
    const size_t from = std::min(_offset, data.size());
    return {data.data() + from, std::min(_length, data.size() - from)};
  }
  std::shared_ptr<BufferedFile> _file;
  size_t _offset, _length;
};

pepp::bts::BufferedFile::BufferedFile(private_ctor_tag, std::string path, std::vector<u8> data, bool readonly)
    : _path(std::move(path)), _readonly(readonly), _data(std::move(data)) {}

std::shared_ptr<pepp::bts::BufferedFile> pepp::bts::BufferedFile::open_readonly(std::string path) {
  auto data = read_all(path);
  if (!data) throw std::runtime_error("BufferedFile: cannot read " + path);
  return std::make_shared<BufferedFile>(private_ctor_tag{}, std::move(path), std::move(*data), true);
}

std::shared_ptr<pepp::bts::BufferedFile> pepp::bts::BufferedFile::open_readwrite(std::string path,
                                                                                 std::optional<u64> size) {
  // Appending creates a missing file without truncating an existing one.
  if (!std::ofstream(path, std::ios::binary | std::ios::app))
    throw std::runtime_error("BufferedFile: cannot create " + path);
  if (size) std::filesystem::resize_file(path, *size);
  auto data = read_all(path);
  if (!data) throw std::runtime_error("BufferedFile: cannot read " + path);
  return std::make_shared<BufferedFile>(private_ctor_tag{}, std::move(path), std::move(*data), false);
}

std::shared_ptr<pepp::bts::BufferedFile> pepp::bts::BufferedFile::open_truncated(std::string path, u64 size) {
  if (!std::ofstream(path, std::ios::binary | std::ios::trunc))
    throw std::runtime_error("BufferedFile: cannot create " + path);
  std::filesystem::resize_file(path, size);
  return std::make_shared<BufferedFile>(private_ctor_tag{}, std::move(path), std::vector<u8>(size), false);
}

std::shared_ptr<pepp::bts::BufferedFile> pepp::bts::BufferedFile::open_anonymous(size_t size) {
  return std::make_shared<BufferedFile>(private_ctor_tag{}, std::string(), std::vector<u8>(size), false);
}

u64 pepp::bts::BufferedFile::size() const { return _data.size(); }

std::shared_ptr<pepp::bts::MappedFile::Slice> pepp::bts::BufferedFile::slice(size_t off, size_t len) noexcept {
  return std::make_shared<BufferedSlice>(std::static_pointer_cast<BufferedFile>(shared_from_this()), off, len);
}

std::shared_ptr<const pepp::bts::MappedFile::Slice> pepp::bts::BufferedFile::slice(size_t off,
                                                                                   size_t len) const noexcept {
  auto self = std::static_pointer_cast<BufferedFile>(const_cast<BufferedFile *>(this)->shared_from_this());
  return std::make_shared<const BufferedSlice>(std::move(self), off, len);
}

void pepp::bts::BufferedFile::write_back(size_t offset, size_t length) const {
  if (_path.empty() || length == 0) return;
  std::fstream file(_path, std::ios::binary | std::ios::in | std::ios::out);
  if (!file) throw std::runtime_error("BufferedFile: cannot open " + _path);
  else if (!file.seekp(static_cast<std::streamoff>(offset)) ||
           !file.write(reinterpret_cast<const char *>(_data.data() + offset), static_cast<std::streamsize>(length)))
    throw std::runtime_error("BufferedFile: cannot write " + _path);
}
