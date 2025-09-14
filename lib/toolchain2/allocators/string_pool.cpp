#include "./string_pool.hpp"
#include <bit>
#include <cstring>
#include <stdexcept>

pepp::tc::alloc::PooledString::PooledString(int16_t page, uint16_t offset, uint16_t length)
    : _page(page), _offset(offset), _length(length) {}

bool pepp::tc::alloc::PooledString::valid() const { return _page != INVALID_PAGE; }

std::strong_ordering pepp::tc::alloc::PooledString::operator<=>(const PooledString &other) const {
  if (auto cmp = _page <=> other._page; cmp != 0) return cmp;
  else if (auto cmp = _offset <=> other._offset; cmp != 0) return cmp;
  return _length <=> other._length;
}

bool pepp::tc::alloc::PooledString::operator==(const PooledString &other) const {
  return _page == other._page && _offset == other._offset && _length == other._length;
}

uint16_t pepp::tc::alloc::PooledString::page() const { return _page; }

uint16_t pepp::tc::alloc::PooledString::offset() const { return _offset; }

uint16_t pepp::tc::alloc::PooledString::length() const { return _length; }

bool pepp::tc::alloc::PooledString::Comparator::operator()(const PooledString &ident_lhs,
                                                           const PooledString &ident_rhs) const {
  auto lhs = context->find(ident_lhs), rhs = context->find(ident_rhs);
  if (!lhs) throw std::invalid_argument("PooledString::Comparator given bad lhs");
  if (!rhs) throw std::invalid_argument("PooledString::Comparator given bad rhs");
  return this->operator()(*lhs, *rhs);
}

bool pepp::tc::alloc::PooledString::Comparator::operator()(const PooledString &ident_lhs, std::string_view rhs) const {
  if (!context) throw std::invalid_argument("PooledString::Comparator context must not be null");
  auto lhs = context->find(ident_lhs);
  if (!lhs) throw std::invalid_argument("PooledString::Comparator given bad lhs");
  return this->operator()(*lhs, rhs);
}

bool pepp::tc::alloc::PooledString::Comparator::operator()(std::string_view lhs, const PooledString &ident_rhs) const {
  if (!context) throw std::invalid_argument("PooledString::Comparator context must not be null");
  auto rhs = context->find(ident_rhs);
  if (!rhs) throw std::invalid_argument("PooledString::Comparator given bad rhs");
  return this->operator()(lhs, *rhs);
}

bool pepp::tc::alloc::PooledString::Comparator::operator()(std::string_view lhs, std::string_view rhs) const {
  if (lhs.size() != rhs.size()) return lhs.size() < rhs.size();
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

pepp::tc::alloc::StringPool::StringPool() : _identifiers(PooledString::Comparator{this}) {}

std::optional<pepp::tc::alloc::PooledString> pepp::tc::alloc::StringPool::find(std::string_view str) const {
  auto item = _identifiers.find(str);
  if (item == _identifiers.end()) return std::nullopt;
  return *item;
}

std::optional<std::string_view> pepp::tc::alloc::StringPool::find(const PooledString &id) const {
  if (!id.valid() || id._page >= _pages.size()) return std::nullopt;
  if (auto &_page = _pages[id._page]; id._offset + id._length > _page.length) return std::nullopt;
  else return std::string_view(&_page.data[id._offset], id._length);
}

bool pepp::tc::alloc::StringPool::contains(std::string_view str) const { return _identifiers.contains(str); }

bool pepp::tc::alloc::StringPool::contains(const PooledString &id) const { return _identifiers.contains(id); }

size_t pepp::tc::alloc::StringPool::count() const { return _identifiers.size(); }

size_t pepp::tc::alloc::StringPool::pooled_byte_size() const {
  size_t ret = 0;
  for (const auto &page : _pages) ret += page.next;
  return ret;
}

size_t pepp::tc::alloc::StringPool::unpooled_byte_size() const {
  size_t ret = 0;
  for (const auto &ident : _identifiers)
    if (auto str = find(ident); str) ret += str->size();
  return ret;
}

pepp::tc::alloc::PooledString pepp::tc::alloc::StringPool::insert(std::string_view str, AddNullTerminator terminator) {
  if (auto existing = _identifiers.find(str); existing != _identifiers.end()) return *existing;
  else if (auto superstring = longest_container_of(str); superstring.valid()) {
    auto superstring_view = find(superstring);
    auto substr_offset = superstring_view->find(str) + superstring._offset;
    return *_identifiers.insert(PooledString(superstring._page, substr_offset, str.size())).first;
  } else return allocate(str, terminator);
}

pepp::tc::alloc::PooledString pepp::tc::alloc::StringPool::longest_container_of(std::string_view target) {
  for (auto it = _identifiers.lower_bound(target); it != _identifiers.cend(); it++)
    if (auto str = find(*it); str && str->find(target) != std::string::npos) return *it;
  return PooledString();
}

pepp::tc::alloc::PooledString pepp::tc::alloc::StringPool::allocate(std::string_view str,
                                                                    AddNullTerminator terminator) {
  // Calculate how long the string is to determine which kind of page to allocate into.
  auto str_length = str.size();
  bool is_null_terminated = !str.empty() && str.back() == '\0';
  bool needs_null_terminator = (terminator == AddNullTerminator::Always) ||
                               (terminator == AddNullTerminator::IfNotPresent && !is_null_terminated);
  if (needs_null_terminator) ++str_length;
  if (str_length >= MAX_PAGE_SIZE) throw std::invalid_argument("String too long to allocate");

  // Check existing pages for space, allocating the string in the first page that has enough space.
  // If I were smarter, I might try different algos other than first-fit to reduce fragmentation.
  for (std::size_t it = 0; it < _pages.size(); ++it)
    if (auto &page = _pages[it]; page.next + str_length <= page.length) {
      auto offset = page.append(str, needs_null_terminator);
      return *_identifiers.insert(PooledString(it, offset, str_length)).first;
    }

  // Allocate a page that is at minimum MIN_PAGE_SIZE, but is rounded up to the next power of 2 if larger.
  // e.g., a 257 byte string will allocate into a 512 byte page.
  // By using a power-of-2 allocation strategy, I hope to reduce the number of page allocations.
  auto page_size = std::bit_ceil(std::max(MIN_PAGE_SIZE, str_length));
  _pages.push_back(Page(page_size, str_length));
  auto &page = _pages.back();
  page.append(str, needs_null_terminator);
  return *_identifiers.insert(PooledString(_pages.size() - 1, 0, str_length)).first;
}

pepp::tc::alloc::StringPool::Page::Page(size_t length, size_t memset_from) : length(length), data(new char[length]) {
  if (length < MIN_PAGE_SIZE) throw std::invalid_argument("Allocation smaller than MIN_PAGE_SIZE");
  else if (length > MAX_PAGE_SIZE) throw std::invalid_argument("Allocation larger than MAX_PAGE_SIZE");
  // Optimization to avoid 0-ing data if you plan on immediately allocating
  else if (memset_from < length) memset(data.get() + memset_from, 0, length - memset_from);
}

size_t pepp::tc::alloc::StringPool::Page::append(std::string_view str, bool add_null_terminator) {
  auto ret = next;
  if (next + str.size() > length) throw std::runtime_error("Page overflow");
  memcpy(&data[next], str.data(), str.size());
  next += str.size();
  if (add_null_terminator) data[next++] = '\0';
  return ret;
}
