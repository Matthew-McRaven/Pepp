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

#if (defined(__unix__) || defined(__APPLE__)) && !defined(__EMSCRIPTEN__)
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include "core/io/mapped_file.hpp"
#include "core/math/bitmanip/log2.hpp"

namespace pepp::bts {
namespace {
// A file memory-mapped with mmap(). The factory opens and maps the whole file, and each slice views a range of that
// mapping.
class PosixMappedFile final : public MappedFile {
public:
  class PosixSlice final : public MappedFile::Slice {
  public:
    PosixSlice(std::shared_ptr<PosixMappedFile> file, size_t offset, size_t length)
        : _file(std::move(file)), _offset(offset), _length(length) {}
    size_t size() const noexcept override { return _length; }
    bool readonly() const noexcept override { return _file->_readonly; }
    bits::span<u8> get() noexcept override { return view(); }
    bits::span<const u8> get() const noexcept override { return view(); }
    void flush() override;

  private:
    // Returns the slice clipped to the file, since touching a mapping past the end of a file faults.
    bits::span<u8> view() const noexcept {
      const size_t from = std::min(_offset, _file->_length);
      return {_file->_base + from, std::min(_length, _file->_length - from)};
    }
    std::shared_ptr<PosixMappedFile> _file;
    size_t _offset, _length;
  };
  PosixMappedFile(private_ctor_tag, std::string path, bool readonly) : _path(std::move(path)), _readonly(readonly) {}
  ~PosixMappedFile() override;

  // Each factory returns nullptr if the file cannot be opened, resized, or mapped.
  static std::shared_ptr<PosixMappedFile> open_readonly(std::string path);
  static std::shared_ptr<PosixMappedFile> open_readwrite(std::string path, std::optional<u64> size);
  static std::shared_ptr<PosixMappedFile> open_truncated(std::string path, u64 size);
  static std::shared_ptr<PosixMappedFile> open_anonymous(size_t size);

  bool memory_mapped() const noexcept override { return true; }
  u64 size() const override { return _length; }
  std::shared_ptr<Slice> slice(size_t off, size_t len) noexcept override;
  std::shared_ptr<const Slice> slice(size_t off, size_t len) const noexcept override;

private:
  std::string _path;
  bool _readonly = false;
  // The mapping made by open() or open_anonymous. Null for an empty file, since mmap does not accept a length of 0.
  u8 *_base = nullptr;
  // The size of the file when it was opened, or of the anonymous allocation.
  size_t _length = 0;

  // Open the file at _path, truncating it if truncate is set. If size is given, resize the file to size bytes. Then map
  // the whole file. Returns false if the file cannot be opened, resized, or mapped.
  bool open(bool truncate, std::optional<u64> size);
};

PosixMappedFile::~PosixMappedFile() {
  if (_base) ::munmap(_base, std::max<size_t>(_length, 1));
}

std::shared_ptr<PosixMappedFile> PosixMappedFile::open_readonly(std::string path) {
  auto ret = std::make_shared<PosixMappedFile>(private_ctor_tag{}, std::move(path), true);
  return ret->open(false, std::nullopt) ? ret : nullptr;
}

std::shared_ptr<PosixMappedFile> PosixMappedFile::open_readwrite(std::string path, std::optional<u64> size) {
  auto ret = std::make_shared<PosixMappedFile>(private_ctor_tag{}, std::move(path), false);
  return ret->open(false, size) ? ret : nullptr;
}

std::shared_ptr<PosixMappedFile> PosixMappedFile::open_truncated(std::string path, u64 size) {
  auto ret = std::make_shared<PosixMappedFile>(private_ctor_tag{}, std::move(path), false);
  return ret->open(true, size) ? ret : nullptr;
}

std::shared_ptr<PosixMappedFile> PosixMappedFile::open_anonymous(size_t size) {
  // mmap does not accept a length of 0.
  const size_t length = std::max<size_t>(size, 1);
  void *base = ::mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (base == MAP_FAILED) {
    spdlog::warn("Failed to mmap {} bytes with errno {}", length, errno);
    return nullptr;
  }
  auto ret = std::make_shared<PosixMappedFile>(private_ctor_tag{}, "", false);
  ret->_base = static_cast<u8 *>(base), ret->_length = size;
  return ret;
}

std::shared_ptr<MappedFile::Slice> PosixMappedFile::slice(size_t off, size_t len) noexcept {
  return std::make_shared<PosixSlice>(std::static_pointer_cast<PosixMappedFile>(shared_from_this()), off, len);
}

std::shared_ptr<const MappedFile::Slice> PosixMappedFile::slice(size_t off, size_t len) const noexcept {
  auto self = std::static_pointer_cast<PosixMappedFile>(const_cast<PosixMappedFile *>(this)->shared_from_this());
  return std::make_shared<const PosixSlice>(std::move(self), off, len);
}

bool PosixMappedFile::open(bool truncate, std::optional<u64> size) {
  const int fd = _readonly ? ::open(_path.c_str(), O_RDONLY)
                           : ::open(_path.c_str(), O_RDWR | O_CREAT | (truncate ? O_TRUNC : 0), 0644);
  if (fd < 0) {
    spdlog::warn("Failed to open file '{}' with errno {}", _path, errno);
    return false;
  }
  bool ok = true;
  struct stat st{};
  if (size && ::ftruncate(fd, static_cast<off_t>(*size)) != 0) {
    spdlog::warn("Failed to resize file '{}' with errno {}", _path, errno);
    ok = false;
  } else if (::fstat(fd, &st) != 0) {
    spdlog::warn("Failed to stat file '{}' with errno {}", _path, errno);
    ok = false;
  } else if (_length = static_cast<size_t>(st.st_size); _length != 0) {
    const int protect = _readonly ? PROT_READ : PROT_READ | PROT_WRITE;
    void *base = ::mmap(nullptr, _length, protect, _readonly ? MAP_PRIVATE : MAP_SHARED, fd, 0);
    if (base == MAP_FAILED) {
      spdlog::warn("Failed to mmap file '{}' with errno {}", _path, errno);
      ok = false;
    } else _base = static_cast<u8 *>(base);
  }
  // Close the descriptor, since the mapping keeps the file open.
  ::close(fd);
  return ok;
}

void PosixMappedFile::PosixSlice::flush() {
  const auto data = view();
  if (_file->_readonly || _file->_path.empty() || data.empty()) return;
  // msync requires a page-aligned address.
  const long ps = ::sysconf(_SC_PAGESIZE);
  const auto page = static_cast<std::uintptr_t>(ps > 0 ? ps : 4096);
  const auto begin = bits::align_down(reinterpret_cast<std::uintptr_t>(data.data()), page);
  const auto end = reinterpret_cast<std::uintptr_t>(data.data() + data.size());
  if (::msync(reinterpret_cast<void *>(begin), end - begin, MS_SYNC) != 0)
    spdlog::warn("Failed to msync file '{}' with errno {}", _file->_path, errno);
}
} // namespace

std::shared_ptr<MappedFile> MappedFile::open_platform(const std::string &path, Mode mode, std::optional<u64> size) {
  switch (mode) {
  case Mode::ReadOnly: return PosixMappedFile::open_readonly(path);
  case Mode::ReadWrite: return PosixMappedFile::open_readwrite(path, size);
  case Mode::Truncate: return PosixMappedFile::open_truncated(path, size.value_or(0));
  case Mode::Anonymous: return PosixMappedFile::open_anonymous(size.value_or(0));
  }
  return nullptr;
}
} // namespace pepp::bts
#endif
