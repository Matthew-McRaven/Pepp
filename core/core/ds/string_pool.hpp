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

#pragma once
#include <optional>
#include <set>
#include <stdint.h>
#include <string_view>
#include <vector>
#include "alloc/paged.hpp"

namespace pepp::bts {

class StringPool;
// A string contained within a StringPool instance.
// While it does not have any methods that look obviously string-like, it is effectively a handle into that StringPool.
// Their primary purpose is to make sorting and comparing pooled strings "cheap". PooledStrings belong to different
// StringPools are not comparable.
struct PooledString {
  PooledString() = default;
  bool valid() const;
  // Sorting works like sorting pointers: page, offset, length.
  // The comparator builds on top of this to sort strings by length for the purpose of sorting the set nicely.
  std::strong_ordering operator<=>(const PooledString &other) const;
  bool operator==(const PooledString &other) const;
  uint16_t page() const;
  uint16_t offset() const;
  uint16_t length() const;

private:
  PooledString(int16_t page, uint16_t offset, uint16_t length);
  friend class StringPool;
  static constexpr uint16_t INVALID_PAGE = -1;
  uint16_t _page = INVALID_PAGE; // If -1/INVALID_PAGE, it is an invalid identifier, otherwise an index into _pages.
  uint16_t _offset = 0;          // Offset into page.data.
  uint16_t _length = 0;          // Length of the identifier, including null terminator if present.
  struct Comparator {
    const StringPool *context = nullptr;
    using is_transparent = std::true_type;
    // Sort by length, then by lexicographical_compare instead of only by lexicography.
    // This is useful for cheaply implemtning longest_suffix_of.
    bool operator()(PooledString lhs, PooledString rhs) const;
    bool operator()(PooledString lhs, std::string_view rhs) const;
    bool operator()(std::string_view lhs, PooledString rhs) const;
    bool operator()(std::string_view lhs, std::string_view rhs) const;
  };
};

/*
 * A collection of string which are de-duplicated to save memory footprint.
 * Operates by allocating large "pages" of memory and sub-allocating strings within those pages.
 * Adding a string to the pool returns a handle (PooledString) which can later be used to retrieve the string.
 * Future insertions of the same string will return the same handle. Strings are immutable once added to the pool
 */
class StringPool {
public:
  static const auto MIN_PAGE_SIZE = PagedAllocator<char>::MIN_PAGE_SIZE;
  static const auto DEFAULT_PAGE_SIZE = PagedAllocator<char>::DEFAULT_PAGE_SIZE;
  static const auto MAX_PAGE_SIZE = PagedAllocator<char>::MAX_PAGE_SIZE;
  using PooledStringSet = std::set<PooledString, PooledString::Comparator>;

  StringPool();

  std::optional<PooledString> find(std::string_view str) const;
  std::optional<std::string_view> find(const PooledString &id) const;
  bool contains(std::string_view str) const;
  bool contains(const PooledString &id) const;
  size_t count() const;

  // The number of bytes required to concatenate all the strings together with the current pooling applied.
  size_t pooled_byte_size() const;
  // Number of bytes required to hold all strings without pooling.
  size_t unpooled_byte_size() const;

  enum class AddNullTerminator { Always, Never, IfNotPresent };

  // Find the longest identifier which str is a suffix of.
  // Returns an invalid identifier if no such identifier exists.
  PooledString longest_container_of(std::string_view str);
  // If str is already in the pool, returns the existing identifier.
  // Otherwise, it attempts to return a substring of an existing identifier.
  // If no substring exists, it will will allocate space for a new string.
  PooledString insert(std::string_view str, AddNullTerminator terminator = AddNullTerminator::Never);

  // Helpers to access underlying pages & identifiers, useful for writing debugger algos that "dump" the string pool.
  std::vector<PagedAllocator<char>::Page>::const_iterator pages_cbegin() const;
  std::vector<PagedAllocator<char>::Page>::const_iterator pages_cend() const;
  PooledStringSet::const_iterator identifiers_cbegin() const;
  PooledStringSet::const_iterator identifiers_cend() const;

private:
  PagedAllocator<char> _allocator = {};
  // Force-allocate space for a new string.
  // Will enforce
  PooledString allocate(std::string_view str, AddNullTerminator terminator);

  // Sort identifiers by string_view so that we can have cheap heterogenous comparisons with string_view
  PooledStringSet _identifiers = {};
};

// A page + the pooled strings within it.
// Not uses within the string pool, but useful for debugging.
/*struct AnnotatedPage {
  const PagedAllocator<char16_t>::Page *page;
  std::vector<PooledString> identifiers;
  QString to_string() const;
};*/
// std::vector<AnnotatedPage> annotated_pages(const StringPool &pool);
} // namespace pepp::core
