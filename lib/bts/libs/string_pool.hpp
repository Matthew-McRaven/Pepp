#pragma once
#include <QString>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <vector>
#include "../bitmanip/integers.h"
#include "../bitmanip/span.hpp"
#include "./paged_alloc.hpp"

namespace pepp::bts {
class StringPool;
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
    bool operator()(PooledString lhs, QStringView rhs) const;
    bool operator()(QStringView lhs, PooledString rhs) const;
    bool operator()(QStringView lhs, QStringView rhs) const;
  };
};

class StringPool {
public:
  static const auto MIN_PAGE_SIZE = PagedAllocator<char16_t>::MIN_PAGE_SIZE;
  static const auto DEFAULT_PAGE_SIZE = PagedAllocator<char16_t>::DEFAULT_PAGE_SIZE;
  static const auto MAX_PAGE_SIZE = PagedAllocator<char16_t>::MAX_PAGE_SIZE;
  using PooledStringSet = std::set<PooledString, PooledString::Comparator>;

  StringPool();

  std::optional<PooledString> find(QStringView str) const;
  std::optional<QStringView> find(const PooledString &id) const;
  bool contains(QStringView str) const;
  bool contains(const PooledString &id) const;
  qsizetype count() const;

  // The number of bytes required to concatenate all the strings together with the current pooling applied.
  qsizetype pooled_byte_size() const;
  // Number of bytes required to hold all strings without pooling.
  qsizetype unpooled_byte_size() const;

  enum class AddNullTerminator { Always, Never, IfNotPresent };

  // Find the longest identifier which str is a suffix of.
  // Returns an invalid identifier if no such identifier exists.
  PooledString longest_container_of(QStringView str);
  // If str is already in the pool, returns the existing identifier.
  // Otherwise, it attempts to return a substring of an existing identifier.
  // If no substring exists, it will will allocate space for a new string.
  PooledString insert(QStringView str, AddNullTerminator terminator = AddNullTerminator::Never);

  // Helpers to access underlying pages & identifiers, useful for writing debugger algos that "dump" the string pool.
  std::vector<PagedAllocator<char16_t>::Page>::const_iterator pages_cbegin() const;
  std::vector<PagedAllocator<char16_t>::Page>::const_iterator pages_cend() const;
  PooledStringSet::const_iterator identifiers_cbegin() const;
  PooledStringSet::const_iterator identifiers_cend() const;

private:
  PagedAllocator<char16_t> _allocator = {};
  // Force-allocate space for a new string.
  // Will enforce
  PooledString allocate(QStringView str, AddNullTerminator terminator);

  // Sort identifiers by string_view so that we can have cheap heterogenous comparisons with string_view
  PooledStringSet _identifiers = {};
};

// A page + the pooled strings within it.
// Not uses within the string pool, but useful for debugging.
struct AnnotatedPage {
  const PagedAllocator<char16_t>::Page *page;
  std::vector<PooledString> identifiers;
  QString to_string() const;
};
// std::vector<AnnotatedPage> annotated_pages(const StringPool &pool);
} // namespace pepp::bts
