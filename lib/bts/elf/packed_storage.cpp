#include "packed_storage.hpp"
#include <system_error>
#include "packed_ops.hpp"
// Needed for memory-mapped storage
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

pepp::bts::AStorage::~AStorage() = default;

size_t pepp::bts::BlockStorage::append(bits::span<const u8> data) {
  auto offset = _storage.size();
  _storage.insert(_storage.end(), data.begin(), data.end());
  return offset;
}

size_t pepp::bts::BlockStorage::allocate(size_t size, u8 fill) {
  auto ret = _storage.size();
  _storage.resize(ret + size, fill);
  return ret;
}

void pepp::bts::BlockStorage::set(size_t offset, bits::span<const u8> data) {
  if (offset + data.size() > _storage.size()) throw std::out_of_range("BlockStorage::set out of range");
  std::memcpy(_storage.data() + offset, data.data(), data.size());
}

bits::span<u8> pepp::bts::BlockStorage::get(size_t offset, size_t length) noexcept {
  if (offset + length > _storage.size()) return {};
  return bits::span<u8>((u8 *)_storage.data() + offset, length);
}

bits::span<const u8> pepp::bts::BlockStorage::get(size_t offset, size_t length) const noexcept {
  if (offset + length > _storage.size()) return {};
  return bits::span<const u8>((const u8 *)_storage.data() + offset, length);
}

size_t pepp::bts::BlockStorage::size() const noexcept { return _storage.size(); }

void pepp::bts::BlockStorage::clear(size_t reserve) {
  _storage.clear();
  if (reserve > 0) _storage.reserve(reserve);
}

size_t pepp::bts::BlockStorage::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  if (_storage.empty()) return dst_offset;
  layout.emplace_back(LayoutItem{dst_offset, bits::span<const u8>{(const u8 *)_storage.data(), _storage.size()}});
  return dst_offset + _storage.size();
}

size_t pepp::bts::BlockStorage::find(bits::span<const u8> needle) const noexcept {
  auto it = std::search(_storage.begin(), _storage.end(), needle.begin(), needle.end());
  return (it == _storage.end()) ? 0 : static_cast<std::size_t>(it - _storage.begin());
}

size_t pepp::bts::BlockStorage::strlen(size_t offset) const noexcept {
  const char *start = (const char *)_storage.data() + offset, *end = start;
  while (end < (const char *)_storage.data() + _storage.size() && *end != '\0') ++end;
  return end - start;
}

size_t pepp::bts::PagedStorage::append(bits::span<const u8> data) {
  if (!_pages.empty() && _pages.back()->can_fit(data)) _pages.back()->append(data);
  else {
    _pages.emplace_back(std::make_unique<Page>(std::max<size_t>(DEFAULT_PAGE_SIZE, data.size())));
    _pages.back()->append(data);
  }
  return std::exchange(_size, _size + data.size());
}

size_t pepp::bts::PagedStorage::allocate(size_t size, u8 fill) {
  if (!_pages.empty() && _pages.back()->can_fit(size)) _pages.back()->allocate(size, fill);
  else {
    _pages.emplace_back(std::make_unique<Page>(std::max<size_t>(DEFAULT_PAGE_SIZE, size)));
    _pages.back()->allocate(size, fill);
  }
  return std::exchange(_size, _size + size);
}

void pepp::bts::PagedStorage::set(size_t offset, bits::span<const u8> data) {
  while (data.size() > 0) {
    auto [page_index, page_offset] = page_for_offset(offset);
    auto &page = _pages[page_index];
    auto to_write = std::min<size_t>(data.size(), page->length - page_offset);
    std::memcpy(page->data.get() + page_offset, data.data(), to_write);
    offset += to_write, data = data.subspan(to_write);
  }
}

bits::span<u8> pepp::bts::PagedStorage::get(size_t offset, size_t length) noexcept {
  if (offset + length >= _size) return {};
  auto [page_index, page_offset] = page_for_offset(offset);
  auto &page = _pages[page_index];
  auto to_read = std::min<size_t>(length, page->length - page_offset);
  return bits::span<u8>(page->data.get() + page_offset, to_read);
}

bits::span<const u8> pepp::bts::PagedStorage::get(size_t offset, size_t length) const noexcept {
  if (offset + length >= _size) return {};
  auto [page_index, page_offset] = page_for_offset(offset);
  auto &page = _pages[page_index];
  auto to_read = std::min<size_t>(length, page->length - page_offset);
  return bits::span<const u8>(page->data.get() + page_offset, to_read);
}

size_t pepp::bts::PagedStorage::size() const noexcept { return _size; }

void pepp::bts::PagedStorage::clear(size_t reserve) {
  for (auto &page : _pages) page->clear();
  if (auto needed_pages = (reserve + MAX_PAGE_SIZE - 1) / MAX_PAGE_SIZE; reserve > 0 && reserve > _size)
    for (u32 it = 0; it < needed_pages - _pages.size(); it++)
      _pages.emplace_back(std::make_unique<Page>(MAX_PAGE_SIZE));
  _size = 0;
}

size_t pepp::bts::PagedStorage::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  if (_pages.empty() || _size == 0) return dst_offset;
  for (const auto &page : _pages) {
    if (page->next == 0) continue;
    layout.emplace_back(
        LayoutItem{dst_offset, bits::span<const u8>{page->data.get(), static_cast<size_t>(page->next)}});
    dst_offset += page->next;
  }
  return dst_offset;
}

size_t pepp::bts::PagedStorage::find(bits::span<const u8> data) const noexcept {
  size_t offset = 0;
  for (const auto &page : _pages) {
    auto it = std::search(page->data.get(), page->data.get() + page->next, data.begin(), data.end());
    if (it != page->data.get() + page->next) return offset + static_cast<size_t>(it - page->data.get());
    offset += page->next;
  }
  return offset;
}

size_t pepp::bts::PagedStorage::strlen(size_t offset) const noexcept {
  if (offset >= _size) return 0;
  auto [start_page_index, start_page_offset] = page_for_offset(offset);
  size_t length = 0;
  while (start_page_index < _pages.size()) {
    auto &page = _pages[start_page_index];
    for (size_t it = start_page_offset; it < page->next; it++)
      if (page->data.get()[it] == '\0') return length;
      else length++;
    start_page_offset = 0, start_page_index++;
    if (start_page_index >= _pages.size()) break;
  }
  return length;
}

std::pair<size_t, size_t> pepp::bts::PagedStorage::page_for_offset(size_t offset) const {
  if (offset >= _size) throw std::runtime_error("Invalid offset!!");
  size_t page_index = 0;
  while (page_index < _pages.size()) {
    if (auto &page = _pages[page_index]; offset < page->next) return {page_index, offset};
    else offset -= page->next, page_index++;
  }
#if defined(_MSC_VER) && !defined(__clang__)
  __assume(false);
#else
  __builtin_unreachable();
#endif
}

pepp::bts::PagedStorage::Page::Page(size_t length, size_t memset_from) : length(length), data(new u8[length]) {
  if (length < MIN_PAGE_SIZE) throw std::invalid_argument("Allocation smaller than MIN_PAGE_SIZE");
  else if (length > MAX_PAGE_SIZE) {
    throw std::invalid_argument("Allocation larger than MAX_PAGE_SIZE");
  }
  // Optimization to avoid 0-ing data if you plan on immediately allocating
  else if (memset_from < length)
    memset(data.get() + memset_from, 0, (length - memset_from) * sizeof(data[0]));
}

size_t pepp::bts::PagedStorage::Page::append(bits::span<const u8> request) {
  if (next + request.size() > length) throw std::runtime_error("Page overflow");
  std::memcpy(&this->data[next], request.data(), request.size());
  return std::exchange(next, next + request.size());
}

size_t pepp::bts::PagedStorage::Page::allocate(size_t request, u8 fill) {
  if (next + request > length) throw std::runtime_error("Page overflow");
  std::memset(&this->data[next], fill, request);
  return std::exchange(next, next + request);
}

pepp::bts::MemoryMapped::MemoryMapped(std::string path, std::size_t off, std::size_t len, bool readonly)
    : path(path), off(off), len(len), readonly(readonly) {}

pepp::bts::MemoryMapped::~MemoryMapped() {
  if (loaded && !readonly) write_fallback();
  release();
}

pepp::bts::MemoryMapped::MemoryMapped(MemoryMapped &&o) noexcept { *this = std::move(o); }

size_t pepp::bts::MemoryMapped::append(bits::span<const u8>) {
  throw std::runtime_error("MemoryMapped storage does not support append()");
}

size_t pepp::bts::MemoryMapped::allocate(size_t, u8) {
  throw std::runtime_error("MemoryMapped storage does not support allocate()");
}

void pepp::bts::MemoryMapped::set(size_t offset, bits::span<const u8> data) {
  if (readonly) return;
  else if (!loaded) load_mapped();
  std::memcpy(const_cast<u8 *>(mapped_view.data()) + offset, data.data(), data.size());
}

bits::span<u8> pepp::bts::MemoryMapped::get(size_t offset, size_t length) noexcept {
  if (!loaded) load_mapped();
  return mapped_view.subspan(offset, length);
}

bits::span<const u8> pepp::bts::MemoryMapped::get(size_t offset, size_t length) const noexcept {
  if (!loaded) load_mapped();
  return mapped_view.subspan(offset, length);
}

size_t pepp::bts::MemoryMapped::size() const noexcept { return len; }

void pepp::bts::MemoryMapped::clear(size_t) {
  if (!loaded) load_mapped();
  std::memset(const_cast<u8 *>(mapped_view.data()), 0, mapped_view.size());
}

size_t pepp::bts::MemoryMapped::calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const {
  if (!loaded) load_mapped();
  layout.emplace_back(LayoutItem{dst_offset, mapped_view});
  return dst_offset + mapped_view.size();
}

size_t pepp::bts::MemoryMapped::find(bits::span<const u8> data) const noexcept {
  if (!loaded) load_mapped();
  auto it = std::search(mapped_view.begin(), mapped_view.end(), data.begin(), data.end());
  if (it == mapped_view.end()) return 0;
  return static_cast<std::size_t>(it - mapped_view.begin());
}

size_t pepp::bts::MemoryMapped::strlen(size_t offset) const noexcept {
  if (!loaded) load_mapped();
  const char *start = reinterpret_cast<const char *>(mapped_view.data() + offset), *end = start;
  while (end < reinterpret_cast<const char *>(mapped_view.data() + mapped_view.size()) && *end != '\0') ++end;
  return end - start;
}

pepp::bts::MemoryMapped &pepp::bts::MemoryMapped::operator=(MemoryMapped &&o) noexcept {
  if (this != &o) {
    release();
    path = std::move(o.path);
    off = o.off, len = o.len;
    o.off = o.len = 0;
    readonly = o.readonly, loaded = o.loaded;
    o.loaded = false;

#if defined(_WIN32)
    hFile = o.hFile, hMap = o.hMap;
    o.hFile = INVALID_HANDLE_VALUE, o.hMap = nullptr;
#elif defined(__unix__) || defined(__APPLE__)
    fd = o.fd;
    o.fd = -1;
#endif
    map_base = o.map_base, map_len = o.map_len;
    o.map_base = nullptr, o.map_len = 0;
    fallback_buf = std::move(o.fallback_buf);
    mapped_view = o.mapped_view;
    o.mapped_view = {};
  }
  return *this;
}

std::size_t pepp::bts::MemoryMapped::page_size() {
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

void pepp::bts::MemoryMapped::load_mapped() const {
#if defined(_WIN32)
  if (readonly)
    hFile = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                          nullptr);
  else
    hFile = ::CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                          nullptr);

  if (hFile == INVALID_HANDLE_VALUE) throw std::system_error(::GetLastError(), std::system_category(), "CreateFile");

  LARGE_INTEGER st{};
  if (!::GetFileSizeEx(hFile, &st)) throw std::system_error(::GetLastError(), std::system_category(), "GetFileSizeEx");

  const u64 file_size = static_cast<u64>(st.QuadPart);

  if (readonly) {
    if (off > file_size || len > file_size - off) throw std::out_of_range("slice exceeds file size");
  } else {
    LARGE_INTEGER li{};
    li.QuadPart = static_cast<LONGLONG>(std::max<u64>(file_size, off + len));
    if (!::SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN))
      throw std::system_error(::GetLastError(), std::system_category(), "SetFilePointerEx");
    if (!::SetEndOfFile(hFile)) throw std::system_error(::GetLastError(), std::system_category(), "SetEndOfFile");
  }

  const std::size_t ps = page_size(), base = off - (off % ps), delta = static_cast<std::size_t>(off - base);
  map_len = delta + static_cast<std::size_t>(len);

  const DWORD protect = readonly ? PAGE_READONLY : PAGE_READWRITE;
  hMap = ::CreateFileMappingA(hFile, nullptr, protect, 0, 0, nullptr);
  if (!hMap) throw std::system_error(::GetLastError(), std::system_category(), "CreateFileMapping");

  ULARGE_INTEGER ubase{};
  ubase.QuadPart = static_cast<unsigned long long>(base);

  const DWORD map_access = readonly ? FILE_MAP_READ : (FILE_MAP_READ | FILE_MAP_WRITE);
  map_base = ::MapViewOfFile(hMap, map_access, ubase.HighPart, ubase.LowPart, map_len);

  if (!map_base) {
    // close handles, clear fields, fallback
    if (hMap) {
      ::CloseHandle(hMap);
      hMap = nullptr;
    }
    if (hFile != INVALID_HANDLE_VALUE) {
      ::CloseHandle(hFile);
      hFile = INVALID_HANDLE_VALUE;
    }
    map_base = nullptr, map_len = 0;
    load_fallback();
  } else mapped_view = {static_cast<u8 *>(map_base) + delta, static_cast<std::size_t>(len)};

#elif defined(__unix__) || defined(__APPLE__)
  struct stat st{};
  if (readonly) fd = ::open(path.c_str(), O_RDONLY);
  else fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);

  if (fd < 0) throw std::system_error(errno, std::generic_category(), "open");
  else if (::fstat(fd, &st) != 0) throw std::system_error(errno, std::generic_category(), "fstat");
  if (readonly) {
    if (auto file_size = static_cast<std::size_t>(st.st_size); off > file_size || len > file_size - off)
      throw std::out_of_range("slice exceeds file size");
  } else {
    if (::ftruncate(fd, std::max<u64>(st.st_size, off + len)) != 0)
      throw std::system_error(errno, std::generic_category(), "ftruncate");
  }

  const std::size_t ps = page_size(), base = off - (off % ps), delta = off - base;
  map_len = delta + len;

  if (readonly) map_base = ::mmap(nullptr, map_len, PROT_READ, MAP_PRIVATE, fd, static_cast<off_t>(base));
  else map_base = ::mmap(nullptr, map_len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, static_cast<off_t>(base));
  if (map_base == MAP_FAILED) {
    if (::close(fd) != 0) throw std::system_error(errno, std::generic_category(), "close after mmap failure");
    fd = -1, map_base = nullptr, map_len = 0; // Unset mmap fields.
    load_fallback();
  } else mapped_view = {static_cast<u8 *>(map_base) + delta, len};
#else
  load_fallback(); // WASM / no mmap
#endif
  loaded = true;
}

void pepp::bts::MemoryMapped::load_fallback() const {
  if (readonly) {
    fallback_buf.resize(len);
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("open failed");
    else if (f.seekg(static_cast<std::streamoff>(off), std::ios::beg); !f) throw std::runtime_error("seek failed");
    else if (f.read(reinterpret_cast<char *>(fallback_buf.data()), static_cast<std::streamsize>(len));
             f.gcount() != static_cast<std::streamsize>(len))
      throw std::runtime_error("short read");
  } else {
    fallback_buf.resize(len, 0);
  }
  mapped_view = {fallback_buf.data(), len};
}

void pepp::bts::MemoryMapped::write_fallback() {
  if (readonly || len == 0) return;
  else if (map_base != nullptr) return; // Do not use fallback if mmaped.
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) throw std::runtime_error("openfailed");
  f.write(reinterpret_cast<const char *>(mapped_view.data()), static_cast<std::streamsize>(len));
  if (!f) throw std::runtime_error("write failed");
  f.close();
}

void pepp::bts::MemoryMapped::release() noexcept {
#if defined(_WIN32)
  if (map_base) {
    ::UnmapViewOfFile(map_base);
    map_base = nullptr, map_len = 0;
  }
  if (hMap) {
    ::CloseHandle(hMap);
    hMap = nullptr;
  }
  if (hFile != INVALID_HANDLE_VALUE) {
    ::CloseHandle(hFile);
    hFile = INVALID_HANDLE_VALUE;
  }
#elif defined(__unix__) || defined(__APPLE__)
  if (map_base) {
    ::munmap(map_base, map_len);
    map_base = nullptr;
    map_len = 0;
  }
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
#endif
  fallback_buf.clear(), mapped_view = {}, loaded = false;
}
