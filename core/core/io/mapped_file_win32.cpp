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
#include <string>
#include "core/io/mapped_file.hpp"
#define NOMINMAX
#include <windows.h>

namespace pepp::bts {
namespace {
// A file memory-mapped with MapViewOfFile(). The factory opens and maps the whole file, and each slice views a range of
// that mapping.
class Win32MappedFile final : public MappedFile {
public:
  class Win32Slice final : public MappedFile::Slice {
  public:
    Win32Slice(std::shared_ptr<Win32MappedFile> file, size_t offset, size_t length)
        : _file(std::move(file)), _offset(offset), _length(length) {}
    size_t size() const noexcept override { return _length; }
    bool readonly() const noexcept override { return _file->_readonly; }
    bits::span<u8> get() noexcept override { return view(); }
    bits::span<const u8> get() const noexcept override { return view(); }
    void flush() override;

  private:
    // Returns the slice clipped to the file, since a view cannot extend past the end of a file.
    bits::span<u8> view() const noexcept {
      const size_t from = std::min(_offset, _file->_length);
      return {_file->_base + from, std::min(_length, _file->_length - from)};
    }
    std::shared_ptr<Win32MappedFile> _file;
    size_t _offset, _length;
  };
  Win32MappedFile(private_ctor_tag, std::string path, bool readonly) : _path(std::move(path)), _readonly(readonly) {}
  ~Win32MappedFile() override;

  // Each factory returns nullptr if the file cannot be opened, resized, or mapped.
  static std::shared_ptr<Win32MappedFile> open_readonly(std::string path);
  static std::shared_ptr<Win32MappedFile> open_readwrite(std::string path, std::optional<u64> size);
  static std::shared_ptr<Win32MappedFile> open_truncated(std::string path, u64 size);
  static std::shared_ptr<Win32MappedFile> open_anonymous(size_t size);

  bool memory_mapped() const noexcept override { return true; }
  u64 size() const override { return _length; }
  std::shared_ptr<Slice> slice(size_t off, size_t len) noexcept override;
  std::shared_ptr<const Slice> slice(size_t off, size_t len) const noexcept override;

private:
  std::string _path;
  bool _readonly = false;
  // The view made by open(), or the allocation made by open_anonymous. Null for an empty file, since Windows refuses to
  // map an empty file.
  u8 *_base = nullptr;
  // The size of the file when it was opened, or of the anonymous allocation.
  size_t _length = 0;

  // Open the file at _path, truncating it if truncate is set. If size is given, resize the file to size bytes. Then map
  // the whole file. Returns false if the file cannot be opened, resized, or mapped.
  bool open(bool truncate, std::optional<u64> size);
};

Win32MappedFile::~Win32MappedFile() {
  if (!_base) return;
  else if (_path.empty()) ::VirtualFree(_base, 0, MEM_RELEASE);
  else ::UnmapViewOfFile(_base);
}

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

std::shared_ptr<Win32MappedFile> Win32MappedFile::open_anonymous(size_t size) {
  // VirtualAlloc does not accept a length of 0.
  const size_t length = std::max<size_t>(size, 1);
  void *base = ::VirtualAlloc(nullptr, length, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!base) {
    spdlog::warn("Failed to VirtualAlloc {} bytes with {}", length, ::GetLastError());
    return nullptr;
  }
  auto ret = std::make_shared<Win32MappedFile>(private_ctor_tag{}, "", false);
  ret->_base = static_cast<u8 *>(base), ret->_length = size;
  return ret;
}

std::shared_ptr<MappedFile::Slice> Win32MappedFile::slice(size_t off, size_t len) noexcept {
  return std::make_shared<Win32Slice>(std::static_pointer_cast<Win32MappedFile>(shared_from_this()), off, len);
}

std::shared_ptr<const MappedFile::Slice> Win32MappedFile::slice(size_t off, size_t len) const noexcept {
  auto self = std::static_pointer_cast<Win32MappedFile>(const_cast<Win32MappedFile *>(this)->shared_from_this());
  return std::make_shared<const Win32Slice>(std::move(self), off, len);
}

bool Win32MappedFile::open(bool truncate, std::optional<u64> size) {
  HANDLE file = _readonly ? ::CreateFileA(_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL, nullptr)
                          : ::CreateFileA(_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                                          truncate ? CREATE_ALWAYS : OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    spdlog::warn("Failed to CreateFileA file '{}' with {}", _path, ::GetLastError());
    return false;
  }
  bool ok = true;
  LARGE_INTEGER length{};
  if (size) {
    LARGE_INTEGER end{};
    end.QuadPart = static_cast<LONGLONG>(*size);
    if (!::SetFilePointerEx(file, end, nullptr, FILE_BEGIN) || !::SetEndOfFile(file)) {
      spdlog::warn("Failed to resize file '{}' with {}", _path, ::GetLastError());
      ok = false;
    }
  }
  if (ok && !::GetFileSizeEx(file, &length)) {
    spdlog::warn("Failed to size file '{}' with {}", _path, ::GetLastError());
    ok = false;
  } else if (ok && (_length = static_cast<size_t>(length.QuadPart)) != 0) {
    // A zero maximum size to CreateFileMappingA means the file's current size.
    HANDLE map = ::CreateFileMappingA(file, nullptr, _readonly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, nullptr);
    if (map) _base = static_cast<u8 *>(::MapViewOfFile(map, _readonly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0));
    if (!_base) {
      spdlog::warn("Failed to map file '{}' with {}", _path, ::GetLastError());
      ok = false;
    }
    // Close both handles, since the view keeps the mapping and the file open.
    if (map) ::CloseHandle(map);
  }
  ::CloseHandle(file);
  return ok;
}

void Win32MappedFile::Win32Slice::flush() {
  const auto data = view();
  if (_file->_readonly || _file->_path.empty() || data.empty()) return;
  else if (!::FlushViewOfFile(data.data(), data.size()))
    spdlog::warn("Failed to FlushViewOfFile file '{}' with {}", _file->_path, ::GetLastError());
}
} // namespace

std::shared_ptr<MappedFile> MappedFile::open_platform(const std::string &path, Mode mode, std::optional<u64> size) {
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
