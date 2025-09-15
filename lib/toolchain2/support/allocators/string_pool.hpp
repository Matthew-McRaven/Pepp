#pragma once
#include <QStringView>
#include <memory>
#include <optional>
#include <set>
#include <stdint.h>
#include <string_view>
#include <vector>

namespace pepp::tc::support {

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

  static constexpr qsizetype MIN_PAGE_SIZE = 256;   // Default allocation size for a single page.
  static constexpr qsizetype MAX_PAGE_SIZE = 65535; // Maximum number of bytes than can be stored in a single page.

  // Holds many strings, one after the other.
  // Lengths will be rounded up to the nearest power-of-2.
  struct Page {
    // Will only set bytes to 0 in the range [memset_from, length)
    // This optimization allows you to avoid a memset when you plan on immediately copying data, saving substantial time
    // on large allocations.
    explicit Page(qsizetype length, qsizetype memset_from = 0);
    qsizetype length = 0, next = 0;
    // Automatically allocated on construction
    std::unique_ptr<char16_t[]> data = nullptr;
    // Copy data into next free space, advancing next.
    qsizetype append(QStringView str, bool add_null_terminator = false);
  };

private:
  // Force-allocate space for a new string.
  // Will enforce
  PooledString allocate(QStringView str, AddNullTerminator terminator);

  static_assert(MIN_PAGE_SIZE <= MAX_PAGE_SIZE, "PAGE_SIZE must fit in uint16_t");
  std::vector<Page> _pages = {};
  // Sort identifiers by string_view so that we can have cheap heterogenous comparisons with string_view
  PooledStringSet _identifiers = {};
};
} // namespace pepp::tc::support
