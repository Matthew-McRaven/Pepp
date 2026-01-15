#include "packed_storage.hpp"
#include "packed_ops.hpp"

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
