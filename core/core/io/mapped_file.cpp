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

#include "mapped_file.hpp"
#include <spdlog/spdlog.h>
#include "core/math/bitmanip/log2.hpp"
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <fstream>

pepp::bts::MappedFile::Slice &pepp::bts::MappedFile::Slice::operator=(Slice &&o) noexcept {
  if (this != &o) {
    release();
    _readonly = o._readonly, _loaded = o._loaded;
    _file_offset = o._file_offset, _file_len = o._file_len, _map_len = o._map_len;
    o._loaded = false, o._file_offset = 0, o._file_len = 0, o._map_len = 0;
#if defined(_WIN32)
    _hMap = o._hMap;
    o._hMap = nullptr;
#endif
    _map_base = o._map_base, _file = std::move(o._file);
    o._map_base = nullptr, o._file = nullptr;
    _fallback_buf = std::move(o._fallback_buf);
    _data_view = o._data_view;
    o._data_view = {};
  }
  return *this;
}

pepp::bts::MappedFile::Slice::~Slice() { release(); }

bits::span<u8> pepp::bts::MappedFile::Slice::get() noexcept {
  ensure_loaded();
  return _data_view;
}

bits::span<const u8> pepp::bts::MappedFile::Slice::get() const noexcept {
  ensure_loaded();
  return _data_view;
}

size_t pepp::bts::MappedFile::Slice::size() const noexcept { return _file_len; }

void pepp::bts::MappedFile::Slice::flush() {
  if (_readonly) return;
  if (_loaded && _file != nullptr) _file->flush(*this);
}

pepp::bts::MappedFile::Slice::Slice(pepp::bts::MappedFile::private_ctor_tag, std::shared_ptr<MappedFile> file,
                                    size_t file_offset, size_t file_len, bool readonly)
    : _readonly(readonly), _loaded(false), _file_offset(file_offset), _file_len(file_len), _map_len(0), _file(file) {}

void pepp::bts::MappedFile::Slice::release() noexcept {
#if defined(_WIN32)
  if (_map_base) {
    ::UnmapViewOfFile(_map_base);
    _map_base = nullptr, _map_len = 0;
  }
  if (_hMap) {
    ::CloseHandle(_hMap);
    _hMap = nullptr;
  }
#elif defined(__unix__) || defined(__APPLE__)
  if (_map_base) {
    ::munmap(_map_base, _map_len);
    _map_base = nullptr, _map_len = 0;
  }
#endif
  _fallback_buf.clear(), _data_view = {}, _loaded = false;
}

pepp::bts::MappedFile &pepp::bts::MappedFile::operator=(MappedFile &&o) noexcept {
  if (this != &o) {
    release();
    _path = std::move(o._path);
    _readonly = o._readonly, _opened = o._opened, _use_fallback = o._use_fallback;
    o._opened = false, o._use_fallback = false;
#if defined(_WIN32)
    _hFile = o._hFile;
    o._hFile = INVALID_HANDLE_VALUE;
#elif defined(__unix__) || defined(__APPLE__)
    _fd = o._fd;
    o._fd = -1;
#endif
  }
  return *this;
}

pepp::bts::MappedFile::~MappedFile() { release(); }

pepp::bts::MappedFile::MappedFile(private_ctor_tag, std::string path, bool readonly)
    : _readonly(readonly), _path(path) {}

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_readonly(std::string path) {
  return std::make_shared<MappedFile>(private_ctor_tag{}, path, true);
}

std::shared_ptr<pepp::bts::MappedFile> pepp::bts::MappedFile::open_readwrite(std::string path) {
  return std::make_shared<MappedFile>(private_ctor_tag{}, path, false);
}

std::shared_ptr<pepp::bts::MappedFile::Slice> pepp::bts::MappedFile::slice(size_t file_offset,
                                                                           size_t file_len) noexcept {
  return std::make_shared<Slice>(private_ctor_tag{}, shared_from_this(), file_offset, file_len, _readonly);
}

std::shared_ptr<const pepp::bts::MappedFile::Slice> pepp::bts::MappedFile::slice(size_t file_offset,
                                                                                 size_t file_len) const noexcept {
  // Need to strip const from this for slice to work correctly.
  auto self = const_cast<MappedFile *>(this)->shared_from_this();
  return std::make_shared<const Slice>(private_ctor_tag{}, self, file_offset, file_len, _readonly);
}

std::size_t pepp::bts::MappedFile::page_size() {
#if defined(_WIN32)
  SYSTEM_INFO si{};
  ::GetSystemInfo(&si);
  return static_cast<std::size_t>(si.dwAllocationGranularity);
#elif defined(__unix__) || defined(__APPLE__)
  long ps = ::sysconf(_SC_PAGESIZE);
  return static_cast<std::size_t>(ps > 0 ? ps : 4096);
#else
  return 4096;
#endif
}

void pepp::bts::MappedFile::open_file() const {
#if defined(_WIN32)
  if (_readonly)
    _hFile = ::CreateFileA(_path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                           nullptr);
  else
    _hFile = ::CreateFileA(_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                           nullptr);

  if (_hFile == INVALID_HANDLE_VALUE) {
    spdlog::warn("Failed to CreateFileA file '{}' with {}", _path, ::GetLastError());
    _use_fallback = true;
    return;
  } else _opened = true;

#elif defined(__unix__) || defined(__APPLE__)
  if (_readonly) _fd = ::open(_path.c_str(), O_RDONLY);
  else _fd = ::open(_path.c_str(), O_RDWR | O_CREAT, 0644);
  if (_fd < 0) {
    spdlog::warn("Failed to open file '{}' with errno {}", _path, errno);
    _use_fallback = true;
    return;
  } else _opened = true;
#else
  _use_fallback = true;
#endif
}

void pepp::bts::MappedFile::release() noexcept {
#if defined(_WIN32)
  if (_hFile != INVALID_HANDLE_VALUE) {
    ::CloseHandle(_hFile);
    _hFile = INVALID_HANDLE_VALUE;
  }
#elif defined(__unix__) || defined(__APPLE__)
  if (_fd >= 0) {
    ::close(_fd);
    _fd = -1;
  }
#endif
}

void pepp::bts::MappedFile::load_mapped(Slice &slice) const {
  if (slice._loaded) return;
  else if (ensure_opened(); _use_fallback) load_fallback(slice);

  const std::size_t ps = page_size(), base = slice._file_offset - (slice._file_offset % ps),
                    delta = slice._file_offset - base;
  slice._map_len = delta + static_cast<std::size_t>(slice._file_len);
#if defined(_WIN32)
  const DWORD protect = _readonly ? PAGE_READONLY : PAGE_READWRITE;
  slice._hMap = ::CreateFileMappingA(_hFile, nullptr, protect, 0, 0, nullptr);
  if (!slice._hMap) {
    spdlog::warn("Failed to CreateFileMapping for file '{}' with {}", _path, ::GetLastError());
    _use_fallback = true;
    slice.release(), load_fallback(slice);
  }

  ULARGE_INTEGER ubase{};
  ubase.QuadPart = static_cast<unsigned long long>(base);

  const DWORD map_access = _readonly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE);
  slice._map_base = ::MapViewOfFile(slice._hMap, map_access, ubase.HighPart, ubase.LowPart, slice._map_len);

  if (!slice._map_base) {
    _use_fallback = true;
    slice.release(), load_fallback(slice);
  } else slice._data_view = {static_cast<u8 *>(slice._map_base) + delta, static_cast<std::size_t>(slice._file_len)};

#elif defined(__unix__) || defined(__APPLE__)
  if (_readonly)
    slice._map_base = ::mmap(nullptr, slice._map_len, PROT_READ, MAP_PRIVATE, _fd, static_cast<off_t>(base));
  else {
    // Try to extend the length of the file if the current slice exceeds max size.
    struct stat st{};
    if (::fstat(_fd, &st) != 0) throw std::system_error(errno, std::generic_category(), "fstat");
    else if (::ftruncate(_fd, std::max<u64>(st.st_size, slice._file_offset + slice._file_len)) != 0)
      throw std::system_error(errno, std::generic_category(), "ftruncate");
    slice._map_base =
        ::mmap(nullptr, slice._map_len, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, static_cast<off_t>(base));
  }
  if (slice._map_base == MAP_FAILED) {
    spdlog::warn("Failed to mmap file '{}' with errno {}", _path, errno);
    if (::close(_fd) != 0) throw std::system_error(errno, std::generic_category(), "close after mmap failure");
    _fd = -1, slice._map_base = nullptr, slice._map_len = 0; // Unset mmap fields.
    load_fallback(slice);
  } else slice._data_view = {static_cast<u8 *>(slice._map_base) + delta, slice._file_len};
#else
  load_fallback(); // WASM / no mmap
#endif
  slice._loaded = true;
}

void pepp::bts::MappedFile::load_fallback(Slice &slice) const {
  if (_readonly) {
    slice._fallback_buf.resize(slice._file_len);
    std::ifstream f(_path, std::ios::binary);
    if (!f) throw std::runtime_error("open failed");
    else if (f.seekg(static_cast<std::streamoff>(slice._file_offset), std::ios::beg); !f)
      throw std::runtime_error("seek failed");
    else if (f.read(reinterpret_cast<char *>(slice._fallback_buf.data()),
                    static_cast<std::streamsize>(slice._fallback_buf.size()));
             f.gcount() != static_cast<std::streamsize>(slice._fallback_buf.size()))
      throw std::runtime_error("short read");
  } else {
    slice._fallback_buf.resize(slice._fallback_buf.size(), 0);
  }
  slice._data_view = {slice._fallback_buf};
}

void pepp::bts::MappedFile::write_fallback(const Slice &slice) {
  if (_readonly || slice._data_view.size() == 0) return;
  else if (slice._map_base != nullptr) return; // Do not use fallback if mmaped.
  std::ofstream f(_path, std::ios::binary | std::ios::trunc);
  f.seekp(slice._file_offset);
  if (!f) throw std::runtime_error("openfailed");
  f.write(reinterpret_cast<const char *>(slice._data_view.data()),
          static_cast<std::streamsize>(slice._data_view.size()));
  if (!f) throw std::runtime_error("write failed");
  f.close();
}

void pepp::bts::MappedFile::flush(const Slice &slice) {
  if (_use_fallback) write_fallback(slice);
  else if (_opened && slice._map_base != nullptr) {
#if defined(_WIN32)
    if (slice._map_base && slice._map_len) {
      if (!::FlushViewOfFile(slice._map_base, slice._map_len))
        spdlog::warn("Failed to FlushViewOfFile file '{}' with {}", _path, ::GetLastError());
    }
#elif defined(__unix__) || defined(__APPLE__)
    const std::size_t ps = page_size();
    auto base = reinterpret_cast<std::uintptr_t>(slice._map_base);
    auto begin = bits::align_down(base, ps);
    auto end = bits::align_up(base + slice._map_len, ps);
    auto *p = reinterpret_cast<void *>(begin);
    auto n = std::size_t(end - begin);

    if (::msync(p, n, MS_SYNC) != 0) spdlog::warn("Failed to msync file '{}' with errno {}", _path, errno);
#endif
  }
}
