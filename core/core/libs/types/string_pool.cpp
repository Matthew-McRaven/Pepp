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

#include "string_pool.hpp"
#include "core/libs/bitmanip/span.hpp"

pepp::bts::PooledString::PooledString(int16_t page, uint16_t offset, uint16_t length)
    : _page(page), _offset(offset), _length(length) {}

bool pepp::bts::PooledString::valid() const { return _page != INVALID_PAGE; }

std::strong_ordering pepp::bts::PooledString::operator<=>(const PooledString &other) const {
  if (auto cmp = _page <=> other._page; cmp != 0) return cmp;
  else if (auto cmp = _offset <=> other._offset; cmp != 0) return cmp;
  return _length <=> other._length;
}

bool pepp::bts::PooledString::operator==(const PooledString &other) const {
  return _page == other._page && _offset == other._offset && _length == other._length;
}

uint16_t pepp::bts::PooledString::page() const { return _page; }

uint16_t pepp::bts::PooledString::offset() const { return _offset; }

uint16_t pepp::bts::PooledString::length() const { return _length; }

bool pepp::bts::PooledString::Comparator::operator()(PooledString ident_lhs, PooledString ident_rhs) const {
  auto lhs = context->find(ident_lhs), rhs = context->find(ident_rhs);
  if (!lhs) throw std::invalid_argument("PooledString::Comparator given bad lhs");
  if (!rhs) throw std::invalid_argument("PooledString::Comparator given bad rhs");
  return this->operator()(*lhs, *rhs);
}

bool pepp::bts::PooledString::Comparator::operator()(PooledString ident_lhs, std::string_view rhs) const {
  if (!context) throw std::invalid_argument("PooledString::Comparator context must not be null");
  auto lhs = context->find(ident_lhs);
  if (!lhs) throw std::invalid_argument("PooledString::Comparator given bad lhs");
  return this->operator()(*lhs, rhs);
}

bool pepp::bts::PooledString::Comparator::operator()(std::string_view lhs, PooledString ident_rhs) const {
  if (!context) throw std::invalid_argument("PooledString::Comparator context must not be null");
  auto rhs = context->find(ident_rhs);
  if (!rhs) throw std::invalid_argument("PooledString::Comparator given bad rhs");
  return this->operator()(lhs, *rhs);
}

bool pepp::bts::PooledString::Comparator::operator()(std::string_view lhs, std::string_view rhs) const {
  if (lhs.size() != rhs.size()) return lhs.size() < rhs.size();
  return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

pepp::bts::StringPool::StringPool() : _identifiers(PooledString::Comparator{this}) {}

std::optional<pepp::bts::PooledString> pepp::bts::StringPool::find(std::string_view str) const {
  auto item = _identifiers.find(str);
  if (item == _identifiers.end()) return std::nullopt;
  return *item;
}

std::optional<std::string_view> pepp::bts::StringPool::find(const PooledString &id) const {
  if (!id.valid() || id._page >= _allocator.page_count()) return std::nullopt;
  if (auto &_page = _allocator.page(id._page); id._offset + id._length > _page.capacity()) return std::nullopt;
  else return std::string_view((char *)&_page.data()[id._offset], id._length);
}

bool pepp::bts::StringPool::contains(std::string_view str) const { return _identifiers.contains(str); }

bool pepp::bts::StringPool::contains(const PooledString &id) const { return _identifiers.contains(id); }

size_t pepp::bts::StringPool::count() const { return _identifiers.size(); }

size_t pepp::bts::StringPool::pooled_byte_size() const { return _allocator.size(); }

size_t pepp::bts::StringPool::unpooled_byte_size() const {
  size_t ret = 0;
  for (const auto &ident : _identifiers)
    if (auto str = find(ident); str) ret += str->size();
  return ret;
}

pepp::bts::PooledString pepp::bts::StringPool::longest_container_of(std::string_view target) {
  for (auto it = _identifiers.lower_bound(target); it != _identifiers.cend(); it++)
    if (auto str = find(*it); str && str->find(target) != std::string_view::npos) return *it;
  return PooledString();
}

pepp::bts::PooledString pepp::bts::StringPool::insert(std::string_view str, AddNullTerminator terminator) {
  if (auto existing = _identifiers.find(str); existing != _identifiers.end()) return *existing;
  else if (auto superstring = longest_container_of(str); superstring.valid()) {
    auto superstring_view = find(superstring);
    auto substr_offset = superstring_view->find(str) + superstring._offset;
    return *_identifiers.insert(PooledString(superstring._page, substr_offset, str.size())).first;
  } else return allocate(str, terminator);
}

using PooledStringSet = pepp::bts::StringPool::PooledStringSet;
PooledStringSet::const_iterator pepp::bts::StringPool::identifiers_cbegin() const { return _identifiers.cbegin(); }

PooledStringSet::const_iterator pepp::bts::StringPool::identifiers_cend() const { return _identifiers.cend(); }

pepp::bts::PooledString pepp::bts::StringPool::allocate(std::string_view str, AddNullTerminator terminator) {
  // Calculate how long the string is to determine which kind of page to allocate into.
  auto str_length = str.size();
  bool is_null_terminated = !str.empty() && str.back() == '\0';
  bool needs_null_terminator = (terminator == AddNullTerminator::Always) ||
                               (terminator == AddNullTerminator::IfNotPresent && !is_null_terminated);
  if (needs_null_terminator) ++str_length;
  if (size_t(str_length) >= PagedAllocator<char>::MAX_PAGE_SIZE)
    throw std::invalid_argument("String too long to allocate");

  // Check existing pages for space, allocating the string in the first page that has enough space.
  // If I were smarter, I might try different algos other than first-fit to reduce fragmentation.
  // TODO: Need to manually insert null terminators here if needs_null_terminator is true.
  PagedAllocator<char>::InsertResult global_index;
  if (needs_null_terminator)
    global_index = _allocator.insert(bits::span<const char>{(const char *)str.data(), (size_t)str.size()}, 0, 1, 0);
  else global_index = _allocator.insert(bits::span<const char>{(const char *)str.data(), (size_t)str.size()});

  return *_identifiers
              .insert(PooledString(static_cast<int16_t>(global_index.indices.index),
                                   static_cast<uint16_t>(global_index.indices.offset),
                                   static_cast<uint16_t>(str_length)))
              .first;
}

/*
QString pepp::core::AnnotatedPage::to_string() const {
  QStringList lines;
  for (const auto &id : identifiers) {
    QStringView str_view = QStringView(&page->data()[id.offset()], id.length());
    lines.push_back(
        QStringLiteral("    [%1,%2]: %3").arg(id.offset(), 4).arg(id.offset() + id.length(), 4).arg(str_view));
  }
  return lines.join('\n');
}

std::vector<pepp::core::AnnotatedPage> pepp::core::annotated_pages(const StringPool &pool) {
  std::vector<AnnotatedPage> ret;
  for (auto it = pool.pages_cbegin(); it != pool.pages_cend(); ++it) ret.push_back(AnnotatedPage{&*it, {}});
  for (auto it = pool.identifiers_cbegin(); it != pool.identifiers_cend(); ++it)
    if (it->page() < ret.size()) ret[it->page()].identifiers.push_back(*it);
  for (auto &page : ret)
    // Sort identifiers by offset rather than by length, which is how they were ordered in the above set.
    std::sort(page.identifiers.begin(), page.identifiers.end(), [](const PooledString &a, const PooledString &b) {
      using namespace sim::api2::memory;
      Interval<int16_t> a_range(a.offset(), a.offset() + a.length());
      Interval<int16_t> b_range(b.offset(), b.offset() + b.length());
      // If one string is contained by the other, sort the longer string first. Then sort by start address.
      if (contains(b_range, a_range)) return false;
      else if (contains(a_range, b_range)) return true;
      else return a_range < b_range;
    });
  return ret;
}*/
