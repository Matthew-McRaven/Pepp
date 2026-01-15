#pragma once
#include "../bitmanip/integers.h"
#include "../bitmanip/span.hpp"
#include "bts/elf/packed_types.hpp"

namespace pepp::bts {
template <class T> struct is_span : std::false_type {};
template <class U, std::size_t Extent> struct is_span<std::span<U, Extent>> : std::true_type {};
template <class T> inline constexpr bool is_span_v = is_span<std::remove_cvref_t<T>>::value;

struct LayoutItem;
struct AStorage {
  virtual ~AStorage() = 0;
  // Guarantees that data will be allocated contiguously in container, even if the underlying storage is fragmented.
  // Returns the offset within the storage where the data was appended.
  virtual size_t append(bits::span<const u8> data) = 0;
  // Allocates `size` uninitialized contiguous bytes and returns the offset.
  virtual size_t allocate(size_t size, u8 fill = 0) = 0;
  // Overwrites data at the given offset, raising an exception if offset+data.size() exceeds current storage.
  virtual void set(size_t offset, bits::span<const u8> data) = 0;
  // Retrieves a span representing the data at the given offset and length. Return nulltpr if offset+length exceeds
  // current storage.
  virtual bits::span<u8> get(size_t offset, size_t length) noexcept = 0;
  virtual bits::span<const u8> get(size_t offset, size_t length) const noexcept = 0;
  virtual size_t size() const noexcept = 0;
  // Clear the underlying storage and attempt to reserve a number of bytes.
  virtual void clear(size_t reserve = 0) = 0;
  // Returns the computed final offset
  virtual size_t calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const = 0;
  virtual size_t find(bits::span<const u8> data) const noexcept = 0;
  virtual size_t strlen(size_t offset) const noexcept = 0;

  // Helpers
  template <std::integral I> void set(size_t offset, I value) {
    set(offset, bits::span<const u8>{(const u8 *)&value, sizeof(I)});
  }
  // Do not enable for integral
  template <typename T>
    requires(!std::is_integral_v<std::remove_cvref_t<T>> && !is_span_v<T>)
  void set(size_t offset, T &&data) {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    return set(offset, bits::span<const u8>((const u8 *)&data, sizeof(T)));
  }

  template <typename T>
    requires(!is_span_v<T>)
  size_t append(T &&data) {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    return append(bits::span<const u8>((const u8 *)&data, sizeof(T)));
  }
  // Assuming the section contains fixed-size entries of type T, get the entry at the given index.
  template <typename T> const T *get(size_t index) const noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(sizeof(T) * index, sizeof(T));
    if (span.size() != sizeof(T)) return nullptr;
    return reinterpret_cast<const T *>(span.data());
  }
  template <typename T> T *get(size_t index) noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(sizeof(T) * index, sizeof(T));
    if (span.size() != sizeof(T)) return nullptr;
    return reinterpret_cast<T *>(span.data());
  }
  // You have an offset into the storage, but your items are not fixed-size so you cannot use get<T>(index)
  template <typename T> const T *get_at(size_t offset) const noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(offset, sizeof(T));
    return reinterpret_cast<const T *>(span.data());
  }
  template <typename T> T *get_at(size_t offset) noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(offset, sizeof(T));
    return reinterpret_cast<T *>(span.data());
  }
  inline bits::span<const u8> get_string_span(size_t index) const noexcept { return get(index, strlen(index)); }
};

// Vector-backed storage
struct BlockStorage : public AStorage {
  // AStorage interface
  size_t append(bits::span<const u8> data) override;
  size_t allocate(size_t size, u8 fill = 0) override;
  void set(size_t offset, bits::span<const u8> data) override;
  bits::span<u8> get(size_t offset, size_t length) noexcept override;
  bits::span<const u8> get(size_t offset, size_t length) const noexcept override;
  size_t size() const noexcept override;
  void clear(size_t reserve = 0) override;
  size_t calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const override;
  size_t find(bits::span<const u8> data) const noexcept override;
  size_t strlen(size_t offset) const noexcept override;

private:
  std::vector<char> _storage{};
};

struct PagedStorage : public AStorage {
  // AStorage interface
  size_t append(bits::span<const u8> data) override;
  size_t allocate(size_t size, u8 fill = 0) override;
  void set(size_t offset, bits::span<const u8> data) override;
  bits::span<u8> get(size_t offset, size_t length) noexcept override;
  bits::span<const u8> get(size_t offset, size_t length) const noexcept override;
  size_t size() const noexcept override;
  void clear(size_t reserve = 0) override;
  size_t calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const override;
  size_t find(bits::span<const u8> data) const noexcept override;
  size_t strlen(size_t offset) const noexcept override;

private:
  static constexpr size_t MIN_PAGE_SIZE = 256;      // Minimum size for a single page.
  static constexpr size_t DEFAULT_PAGE_SIZE = 4096; // Default allocation size for a single page.
  static constexpr size_t MAX_PAGE_SIZE = 65535;    // Maximum number of bytes than can be stored in a single page.

  // Holds many strings, one after the other.
  // Lengths will be rounded up to the nearest power-of-2.
  struct Page {
    // Will only set bytes to 0 in the range [memset_from, length)
    // This optimization allows you to avoid a memset when you plan on immediately copying data, saving substantial time
    // on large allocations.
    explicit Page(size_t length, size_t memset_from = 0);
    size_t length = 0, next = 0;
    // Automatically allocated on construction
    std::unique_ptr<u8[]> data = nullptr;
    // Copy data into next free space, advancing next.
    size_t append(bits::span<const u8> data);
    size_t allocate(size_t size, u8 fill = 0);
    inline bool can_fit(bits::span<const u8> request) const noexcept { return next + request.size() <= length; }
    inline bool can_fit(size_t request) const noexcept { return next + request <= length; }
    inline void clear() noexcept { next = 0, memset(data.get(), 0, length); }
  };
  // Page index, page offset
  std::pair<size_t, size_t> page_for_offset(size_t offset) const;

  static_assert(MIN_PAGE_SIZE <= MAX_PAGE_SIZE, "PAGE_SIZE must fit in u16");
  std::vector<std::unique_ptr<Page>> _pages = {};
  // Number of bytes actually in use
  size_t _size = 0;
};

struct MemoryMapped : public AStorage {
  MemoryMapped(std::string path, std::size_t off, std::size_t len, bool readonly = true);
  ~MemoryMapped() override;
  MemoryMapped(const MemoryMapped &) = delete;
  MemoryMapped &operator=(const MemoryMapped &) = delete;
  MemoryMapped(MemoryMapped &&o) noexcept;
  MemoryMapped &operator=(MemoryMapped &&o) noexcept;

  // AStorage interface
  size_t append(bits::span<const u8> data) override;
  size_t allocate(size_t size, u8 fill = 0) override;
  void set(size_t offset, bits::span<const u8> data) override;
  bits::span<u8> get(size_t offset, size_t length) noexcept override;
  bits::span<const u8> get(size_t offset, size_t length) const noexcept override;
  size_t size() const noexcept override;
  void clear(size_t reserve = 0) override;
  size_t calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const override;
  size_t find(bits::span<const u8> data) const noexcept override;
  size_t strlen(size_t offset) const noexcept override;

private:
  std::string path = 0;
  size_t off = 0, len = 0;
  bool readonly = false;
  mutable bool loaded = false;
  mutable std::span<u8> mapped_view{};
  mutable std::vector<u8> fallback_buf{};
#if defined(_WIN32)
  mutable void* hFile = nullptr;
  mutable void* hMap = nullptr;
#elif defined(__unix__) || defined(__APPLE__)
  mutable int fd = -1;
#endif
  mutable void *map_base = nullptr;
  mutable std::size_t map_len = 0;

  static std::size_t page_size();
  void load_mapped() const;
  void load_fallback() const;
  void write_fallback();
  void release() noexcept;
};
} // namespace pepp::bts
