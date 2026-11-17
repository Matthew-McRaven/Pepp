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
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include "core/io/mapped_file.hpp"
#include "core/math/bitmanip/log2.hpp"

namespace pepp::bts {
namespace {
// A file memory-mapped with mmap(). The factory opens the file, and each slice maps its range on first access.
// I think Slices must be released before the containing PosixMappedFile is destroyed because the fd must remain valid
// for the lifetime of a mapping, but I'm frankly unsure.
class PosixMappedFile final : public MappedFile {
public:
  class PosixSlice final : public MappedFile::Slice {
  public:
    // Public to only make_shared to work, but should be treated as-if private.
    // It takes a shared_ptr to a "containing" mapped to ensure that the fd remains open longer than all mappings are
    // active.
    PosixSlice(private_ctor_tag, std::shared_ptr<PosixMappedFile> file, size_t file_offset, size_t file_len,
               bool readonly);
    // Must release() in dtor.
    ~PosixSlice() override;

    // Methods that do not trigger a load.
    size_t size() const noexcept override { return _file_len; }
    bool readonly() const noexcept override { return _readonly; }

    // Methods that trigger a load.
    bits::span<u8> get() noexcept override;
    bits::span<const u8> get() const noexcept override;
    // Ensure any changes are flushed to the containing PosixMappedFile.
    void flush() override;

  private:
    void ensure_loaded() const {
      if (!_loaded && _file != nullptr) _file->load_mapped(*const_cast<PosixSlice *>(this));
    }
    // Perform platform-specific release of mapped memory within this class.
    void release() noexcept;
    friend class PosixMappedFile;
    bool _readonly = false;
    mutable bool _loaded = false;
    std::size_t _file_offset = 0, _file_len = 0;
    mutable std::size_t _map_len = 0;
    mutable void *_map_base = nullptr;
    std::shared_ptr<PosixMappedFile> _file = nullptr;
    mutable bits::span<u8> _data_view = {};
  };
  PosixMappedFile() = delete;
  // Public to only make_shared to work, but should be treated as-if private.
  PosixMappedFile(private_ctor_tag, std::string path, bool readonly);
  ~PosixMappedFile() override;

  // Each factory returns nullptr if the file cannot be opened or resized, or memory cannot be mapped.
  static std::shared_ptr<PosixMappedFile> open_readonly(std::string path);
  static std::shared_ptr<PosixMappedFile> open_readwrite(std::string path, std::optional<u64> size);
  static std::shared_ptr<PosixMappedFile> open_truncated(std::string path, u64 size);
  static std::shared_ptr<PosixMappedFile> open_anonymous(size_t size);

  bool memory_mapped() const noexcept override { return true; }
  u64 size() const override;
  std::shared_ptr<Slice> slice(size_t off, size_t len) noexcept override;
  std::shared_ptr<const Slice> slice(size_t off, size_t len) const noexcept override;

private:
  bool _readonly = false;
  std::string _path = "";
  // The mapping made by open_anonymous, which is null for a file.
  u8 *_anonymous = nullptr;
  size_t _anonymous_len = 0;
  int _fd = -1;

  // Open the file at _path, truncating it if truncate is set. If size is given, resize the file to size bytes. Returns
  // false if the file cannot be opened or resized.
  bool open(bool truncate, std::optional<u64> size);
  // Returns the size of the open file, or nullopt if it cannot be determined.
  std::optional<u64> open_file_size() const noexcept;
  // Close the underlying file. I'm pretty sure this is only safe to call when no mapped regions are active.
  // So, please only call it from the DTOR.
  void release() noexcept;

  // Returns the size of an OS page so that we can page-align our loads
  static std::size_t page_size();

  // Helpers to reach into the guts of a slice and read/write its contents.
  void load_mapped(PosixSlice &) const;
  void flush(const PosixSlice &);
};

PosixMappedFile::PosixSlice::PosixSlice(private_ctor_tag, std::shared_ptr<PosixMappedFile> file, size_t file_offset,
                                        size_t file_len, bool readonly)
    : _readonly(readonly), _file_offset(file_offset), _file_len(file_len), _file(std::move(file)) {}

PosixMappedFile::PosixSlice::~PosixSlice() { release(); }

bits::span<u8> PosixMappedFile::PosixSlice::get() noexcept {
  ensure_loaded();
  return _data_view;
}

bits::span<const u8> PosixMappedFile::PosixSlice::get() const noexcept {
  ensure_loaded();
  return _data_view;
}

void PosixMappedFile::PosixSlice::flush() {
  if (!_readonly && _loaded && _file != nullptr) _file->flush(*this);
}

PosixMappedFile::PosixMappedFile(private_ctor_tag, std::string path, bool readonly)
    : _readonly(readonly), _path(std::move(path)) {}

PosixMappedFile::~PosixMappedFile() { release(); }

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

u64 PosixMappedFile::size() const {
  if (_anonymous) return _anonymous_len;
  else if (const auto ret = open_file_size()) return *ret;
  throw std::runtime_error("PosixMappedFile: cannot determine the size of " + _path);
}

std::shared_ptr<MappedFile::Slice> PosixMappedFile::slice(size_t file_offset, size_t file_len) noexcept {
  auto self = std::static_pointer_cast<PosixMappedFile>(shared_from_this());
  return std::make_shared<PosixSlice>(private_ctor_tag{}, std::move(self), file_offset, file_len, _readonly);
}

std::shared_ptr<const MappedFile::Slice> PosixMappedFile::slice(size_t file_offset, size_t file_len) const noexcept {
  // Need to strip const from this for slice to work correctly.
  auto self = std::static_pointer_cast<PosixMappedFile>(const_cast<PosixMappedFile *>(this)->shared_from_this());
  return std::make_shared<const PosixSlice>(private_ctor_tag{}, std::move(self), file_offset, file_len, _readonly);
}

void PosixMappedFile::PosixSlice::release() noexcept {
  if (_map_base) {
    ::munmap(_map_base, _map_len);
    _map_base = nullptr, _map_len = 0;
  }
  _data_view = {}, _loaded = false;
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
  ret->_anonymous = static_cast<u8 *>(base), ret->_anonymous_len = size;
  return ret;
}

std::size_t PosixMappedFile::page_size() {
  long ps = ::sysconf(_SC_PAGESIZE);
  return static_cast<std::size_t>(ps > 0 ? ps : 4096);
}

bool PosixMappedFile::open(bool truncate, std::optional<u64> size) {
  if (_readonly) _fd = ::open(_path.c_str(), O_RDONLY);
  else _fd = ::open(_path.c_str(), O_RDWR | O_CREAT | (truncate ? O_TRUNC : 0), 0644);
  if (_fd < 0) {
    spdlog::warn("Failed to open file '{}' with errno {}", _path, errno);
    return false;
  } else if (!size || ::ftruncate(_fd, static_cast<off_t>(*size)) == 0) return true;
  spdlog::warn("Failed to resize file '{}' with errno {}", _path, errno);
  return false;
}

std::optional<u64> PosixMappedFile::open_file_size() const noexcept {
  struct stat st{};
  if (::fstat(_fd, &st) == 0) return static_cast<u64>(st.st_size);
  return std::nullopt;
}

void PosixMappedFile::release() noexcept {
  if (_anonymous) {
    ::munmap(_anonymous, std::max<size_t>(_anonymous_len, 1));
    _anonymous = nullptr, _anonymous_len = 0;
  }
  if (_fd >= 0) {
    ::close(_fd);
    _fd = -1;
  }
}

void PosixMappedFile::load_mapped(PosixSlice &slice) const {
  if (slice._loaded) return;
  // Clip the slice to the file, since touching a mapping past the end of a file faults.
  const u64 file_size = _anonymous ? _anonymous_len : open_file_size().value_or(0);
  const size_t offset = std::min<u64>(slice._file_offset, file_size);
  const size_t length = std::min<u64>(slice._file_len, file_size - offset);
  slice._loaded = true;
  // Anonymous mapping is eagerly loaded and shared between all slices.
  if (_anonymous) {
    slice._data_view = {_anonymous + offset, length};
    return;
  }
  // mmap does not accept a length of 0.
  else if (length == 0) return;

  const std::size_t ps = page_size(), base = offset - (offset % ps), delta = offset - base;
  slice._map_len = delta + length;
  const int protect = _readonly ? PROT_READ : PROT_READ | PROT_WRITE;
  slice._map_base =
      ::mmap(nullptr, slice._map_len, protect, _readonly ? MAP_PRIVATE : MAP_SHARED, _fd, static_cast<off_t>(base));
  if (slice._map_base == MAP_FAILED) {
    spdlog::warn("Failed to mmap file '{}' with errno {}", _path, errno);
    slice._map_base = nullptr, slice._map_len = 0;
  } else slice._data_view = {static_cast<u8 *>(slice._map_base) + delta, length};
}

void PosixMappedFile::flush(const PosixSlice &slice) {
  if (slice._map_base == nullptr) return;
  const std::size_t ps = page_size();
  auto base = reinterpret_cast<std::uintptr_t>(slice._map_base);
  auto begin = bits::align_down(base, ps);
  auto end = bits::align_up(base + slice._map_len, ps);
  auto *p = reinterpret_cast<void *>(begin);
  auto n = std::size_t(end - begin);

  if (::msync(p, n, MS_SYNC) != 0) spdlog::warn("Failed to msync file '{}' with errno {}", _path, errno);
}

} // namespace

std::shared_ptr<MappedFile> MappedFile::open_platform(const std::string &path, Mode mode,
                                                      std::optional<u64> size) {
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
