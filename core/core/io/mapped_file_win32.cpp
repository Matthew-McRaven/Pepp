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

#if defined(_WIN32)
#include <spdlog/spdlog.h>
#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include "core/io/mapped_file.hpp"
#define NOMINMAX
#include <windows.h>

namespace pepp::bts {
namespace {
// A file memory-mapped with MapViewOfFile(). The factory opens the file, and each slice maps its range on first access.
// I think Slices must be released before the containing Win32MappedFile is destroyed because the handle must remain
// valid for the lifetime of a mapping, but I'm frankly unsure.
class Win32MappedFile final : public MappedFile {
public:
  class Win32Slice final : public MappedFile::Slice {
  public:
    // Public to only make_shared to work, but should be treated as-if private.
    // It takes a shared_ptr to a "containing" mapped to ensure that the fd remains open longer than all mappings are
    // active.
    Win32Slice(private_ctor_tag, std::shared_ptr<Win32MappedFile> file, size_t file_offset, size_t file_len,
               bool readonly);
    // Must release() in dtor.
    ~Win32Slice() override;

    // Methods that do not trigger a load.
    size_t size() const noexcept override { return _file_len; }
    bool readonly() const noexcept override { return _readonly; }

    // Methods that trigger a load.
    bits::span<u8> get() noexcept override;
    bits::span<const u8> get() const noexcept override;
    // Ensure any changes are flushed to the containing Win32MappedFile.
    void flush() override;

  private:
    void ensure_loaded() const {
      if (!_loaded && _file != nullptr) _file->load_mapped(*const_cast<Win32Slice *>(this));
    }
    // Perform platform-specific release of mapped memory within this class.
    void release() noexcept;
    friend class Win32MappedFile;
    bool _readonly = false;
    mutable bool _loaded = false;
    std::size_t _file_offset = 0, _file_len = 0;
    mutable std::size_t _map_len = 0;
    mutable void *_hMap = nullptr;
    mutable void *_map_base = nullptr;
    std::shared_ptr<Win32MappedFile> _file = nullptr;
    mutable bits::span<u8> _data_view = {};
  };
  Win32MappedFile() = delete;
  // Public to only make_shared to work, but should be treated as-if private.
  Win32MappedFile(private_ctor_tag, std::string path, bool readonly);
  ~Win32MappedFile() override;

  // Each factory returns nullptr if the file cannot be opened or resized, or memory cannot be mapped.
  static std::shared_ptr<Win32MappedFile> open_readonly(std::string path);
  static std::shared_ptr<Win32MappedFile> open_readwrite(std::string path, std::optional<u64> size);
  static std::shared_ptr<Win32MappedFile> open_truncated(std::string path, u64 size);
  static std::shared_ptr<Win32MappedFile> open_anonymous(size_t size);

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
  // A HANDLE. An anonymous file has none.
  void *_hFile = nullptr;

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
  void load_mapped(Win32Slice &) const;
  void flush(const Win32Slice &);
};

Win32MappedFile::Win32Slice::Win32Slice(private_ctor_tag, std::shared_ptr<Win32MappedFile> file, size_t file_offset,
                                        size_t file_len, bool readonly)
    : _readonly(readonly), _file_offset(file_offset), _file_len(file_len), _file(std::move(file)) {}

Win32MappedFile::Win32Slice::~Win32Slice() { release(); }

bits::span<u8> Win32MappedFile::Win32Slice::get() noexcept {
  ensure_loaded();
  return _data_view;
}

bits::span<const u8> Win32MappedFile::Win32Slice::get() const noexcept {
  ensure_loaded();
  return _data_view;
}

void Win32MappedFile::Win32Slice::flush() {
  if (!_readonly && _loaded && _file != nullptr) _file->flush(*this);
}

Win32MappedFile::Win32MappedFile(private_ctor_tag, std::string path, bool readonly)
    : _readonly(readonly), _path(std::move(path)) {}

Win32MappedFile::~Win32MappedFile() { release(); }

std::shared_ptr<Win32MappedFile> Win32MappedFile::open_readonly(std::string path) {
  auto ret = std::make_shared<Win32MappedFile>(private_ctor_tag{}, std::move(path), true);
  return ret->open(false, std::nullopt) ? ret : nullptr;
}

std::shared_ptr<Win32MappedFile> Win32MappedFile::open_readwrite(std::string path, std::optional<u64> size) {
  auto ret = std::make_shared<Win32MappedFile>(private_ctor_tag{}, std::move(path), false);
  return ret->open(false, size) ? ret : nullptr;
}

std::shared_ptr<Win32MappedFile> Win32MappedFile::open_truncated(std::string path, u64 size) {
  auto ret = std::make_shared<Win32MappedFile>(private_ctor_tag{}, std::move(path), false);
  return ret->open(true, size) ? ret : nullptr;
}

u64 Win32MappedFile::size() const {
  if (_anonymous) return _anonymous_len;
  else if (const auto ret = open_file_size()) return *ret;
  throw std::runtime_error("Win32MappedFile: cannot determine the size of " + _path);
}

std::shared_ptr<MappedFile::Slice> Win32MappedFile::slice(size_t file_offset, size_t file_len) noexcept {
  auto self = std::static_pointer_cast<Win32MappedFile>(shared_from_this());
  return std::make_shared<Win32Slice>(private_ctor_tag{}, std::move(self), file_offset, file_len, _readonly);
}

std::shared_ptr<const MappedFile::Slice> Win32MappedFile::slice(size_t file_offset, size_t file_len) const noexcept {
  // Need to strip const from this for slice to work correctly.
  auto self = std::static_pointer_cast<Win32MappedFile>(const_cast<Win32MappedFile *>(this)->shared_from_this());
  return std::make_shared<const Win32Slice>(private_ctor_tag{}, std::move(self), file_offset, file_len, _readonly);
}

void Win32MappedFile::Win32Slice::release() noexcept {
  if (_map_base) {
    ::UnmapViewOfFile(_map_base);
    _map_base = nullptr, _map_len = 0;
  }
  if (_hMap) {
    ::CloseHandle(_hMap);
    _hMap = nullptr;
  }
  _data_view = {}, _loaded = false;
}

std::shared_ptr<Win32MappedFile> Win32MappedFile::open_anonymous(size_t size) {
  // VirtualAlloc does not accept a length of 0.
  const size_t length = std::max<size_t>(size, 1);
  void *base = ::VirtualAlloc(nullptr, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!base) {
    spdlog::warn("Failed to VirtualAlloc {} bytes with {}", length, ::GetLastError());
    return nullptr;
  }
  auto ret = std::make_shared<Win32MappedFile>(private_ctor_tag{}, "", false);
  ret->_anonymous = static_cast<u8 *>(base), ret->_anonymous_len = size;
  return ret;
}

std::size_t Win32MappedFile::page_size() {
  SYSTEM_INFO si{};
  ::GetSystemInfo(&si);
  return static_cast<std::size_t>(si.dwAllocationGranularity);
}

bool Win32MappedFile::open(bool truncate, std::optional<u64> size) {
  if (_readonly)
    _hFile = ::CreateFileA(_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                           nullptr);
  else
    _hFile = ::CreateFileA(_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                           truncate ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (_hFile == INVALID_HANDLE_VALUE) {
    spdlog::warn("Failed to CreateFileA file '{}' with {}", _path, ::GetLastError());
    return false;
  } else if (!size) return true;
  LARGE_INTEGER end{};
  end.QuadPart = static_cast<LONGLONG>(*size);
  if (::SetFilePointerEx(_hFile, end, nullptr, FILE_BEGIN) && ::SetEndOfFile(_hFile)) return true;
  spdlog::warn("Failed to resize file '{}' with {}", _path, ::GetLastError());
  return false;
}

std::optional<u64> Win32MappedFile::open_file_size() const noexcept {
  LARGE_INTEGER size{};
  if (::GetFileSizeEx(_hFile, &size)) return static_cast<u64>(size.QuadPart);
  return std::nullopt;
}

void Win32MappedFile::release() noexcept {
  if (_anonymous) {
    ::VirtualFree(_anonymous, 0, MEM_RELEASE);
    _anonymous = nullptr, _anonymous_len = 0;
  }
  if (_hFile != nullptr && _hFile != INVALID_HANDLE_VALUE) ::CloseHandle(_hFile);
  _hFile = nullptr;
}

void Win32MappedFile::load_mapped(Win32Slice &slice) const {
  if (slice._loaded) return;
  // Clip the slice to the file, since a view cannot extend past the end of a file.
  const u64 file_size = _anonymous ? _anonymous_len : open_file_size().value_or(0);
  const size_t offset = std::min<u64>(slice._file_offset, file_size);
  const size_t length = std::min<u64>(slice._file_len, file_size - offset);
  slice._loaded = true;
  // Anonymous mapping is eagerly loaded and shared between all slices.
  if (_anonymous) {
    slice._data_view = {_anonymous + offset, length};
    return;
  }
  // Windows refuses to map an empty file or view.
  else if (length == 0) return;

  const std::size_t ps = page_size(), base = offset - (offset % ps), delta = offset - base;
  slice._map_len = delta + length;
  // A zero maximum size to CreateFileMappingA means the file's current size, which covers the clipped slice.
  const DWORD protect = _readonly ? PAGE_READONLY : PAGE_READWRITE;
  slice._hMap = ::CreateFileMappingA(_hFile, nullptr, protect, 0, 0, nullptr);
  if (slice._hMap) {
    ULARGE_INTEGER ubase{};
    ubase.QuadPart = static_cast<unsigned long long>(base);
    const DWORD map_access = _readonly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE);
    slice._map_base = ::MapViewOfFile(slice._hMap, map_access, ubase.HighPart, ubase.LowPart, slice._map_len);
  }
  if (slice._map_base) slice._data_view = {static_cast<u8 *>(slice._map_base) + delta, length};
  else {
    spdlog::warn("Failed to map file '{}' with {}", _path, ::GetLastError());
    slice.release();
    // A failed mapping must not be retried on every get().
    slice._loaded = true;
  }
}

void Win32MappedFile::flush(const Win32Slice &slice) {
  if (slice._map_base && !::FlushViewOfFile(slice._map_base, slice._map_len))
    spdlog::warn("Failed to FlushViewOfFile file '{}' with {}", _path, ::GetLastError());
}

} // namespace

std::shared_ptr<MappedFile> MappedFile::open_platform(const std::string &path, Mode mode,
                                                      std::optional<u64> size) {
  switch (mode) {
  case Mode::ReadOnly: return Win32MappedFile::open_readonly(path);
  case Mode::ReadWrite: return Win32MappedFile::open_readwrite(path, size);
  case Mode::Truncate: return Win32MappedFile::open_truncated(path, size.value_or(0));
  case Mode::Anonymous: return Win32MappedFile::open_anonymous(size.value_or(0));
  }
  return nullptr;
}
} // namespace pepp::bts
#endif
