#pragma once
#include <set>
#include <stdint.h>
#include <vector>

namespace pepp::tc::alloc {

class StringPool;
struct PooledString {
  PooledString() = default;
  bool valid() const;
  // Sort by length first, then page, then offset.
  std::strong_ordering operator<=>(const PooledString &other) const;
  bool operator==(const PooledString &other) const;
  operator bool() const;

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
    bool operator()(const PooledString &lhs, const PooledString &rhs) const;
    bool operator()(const PooledString &lhs, std::string_view rhs) const;
    bool operator()(std::string_view lhs, const PooledString &rhs) const;
    bool operator()(std::string_view lhs, std::string_view rhs) const;
  };
};

class StringPool {
public:
  using PooledStringSet = std::set<PooledString, PooledString::Comparator>;
  using const_iterator = PooledStringSet::const_iterator;
  StringPool();

  const_iterator begin();
  const_iterator end();
  const_iterator cbegin() const;
  const_iterator cend() const;
  std::optional<pepp::tc::alloc::PooledString> find(std::string_view str) const;
  std::optional<std::string_view> find(const PooledString &id) const;
  bool contains(std::string_view str) const;
  bool contains(const PooledString &id) const;

  enum class AddNullTerminator { Always, Never, IfNotPresent };

  // If str is already in the pool, returns the existing identifier.
  // Otherwise, it attempts to return a substring of an existing identifier.
  // If no substring exists, it will will allocate space for a new string.
  PooledString insert(std::string_view str, AddNullTerminator terminator = AddNullTerminator::IfNotPresent);
  // Find the longest identifier which str is a suffix of.
  // Returns an invalid identifier if no such identifier exists.
  PooledString longest_suffix_of(std::string_view str);

private:
  PooledString allocate(std::string_view str, AddNullTerminator terminator);
  static constexpr size_t MIN_PAGE_SIZE = 256;   // Default allocation size for a single page.
  static constexpr size_t MAX_PAGE_SIZE = 65535; // Maximum number of bytes than can be stored in a single page.
  static_assert(MIN_PAGE_SIZE <= MAX_PAGE_SIZE, "PAGE_SIZE must fit in uint16_t");

  // Holds many strings, one after the other.
  // Lengths will be rounded up to the nearest power-of-2.
  struct Page {
    // Will only set bytes to 0 in the range [memset_from, length)
    // This optimization allows you to avoid a memset when you plan on immediately copying data, saving substantial time
    // on large allocations.
    explicit Page(size_t length, size_t memset_from = 0);
    size_t length = 0, next = 0;
    // Automatically allocated on construction
    std::unique_ptr<char[]> data = nullptr;
    // Copy data into next free space, advancing next.
    size_t append(std::string_view str, bool add_null_terminator = false);
  };

  std::vector<Page> _pages = {};
  // Sort identifiers by string_view so that we can have cheap heterogenous comparisons with string_view
  PooledStringSet _identifiers = {};
};
} // namespace pepp::tc::alloc
