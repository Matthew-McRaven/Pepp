#pragma once
#include "../bitmanip/integers.h"
#include "../bitmanip/span.hpp"
#include "bts/elf/packed_types.hpp"
#include "bts/libs/mapped_file.hpp"
#include "bts/libs/paged_alloc.hpp"

namespace pepp::bts {
template <class T> struct is_span : std::false_type {};
template <class U, std::size_t Extent> struct is_span<std::span<U, Extent>> : std::true_type {};
template <class T> inline constexpr bool is_span_v = is_span<std::remove_cvref_t<T>>::value;

struct LayoutItem;
struct AStorage {
  virtual ~AStorage() = 0;
  /*======================
   *= Element Creation   =
   *======================*/
  // Guarantees that data will be allocated contiguously in container, even if the underlying storage is fragmented.
  // Returns the offset within the storage where the data was appended.
  virtual size_t append(bits::span<const u8> data) = 0;
  // Allocates `size` uninitialized contiguous bytes and returns the offset.
  virtual size_t allocate(size_t size, u8 fill = 0) = 0;
  /*======================
   *= Element Read/Write =
   *======================*/
  // Overwrites data at the given offset, raising an exception if offset+data.size() exceeds current storage.
  virtual void set(size_t offset, bits::span<const u8> data) = 0;
  // Retrieves a span representing the data at the given offset and length. Return nulltpr if offset+length either: 1)
  // exceeds underlying capactiy, or 2) is not laid out contiguously in memory .
  virtual bits::span<u8> get(size_t offset, size_t length) noexcept = 0;
  virtual bits::span<const u8> get(size_t offset, size_t length) const noexcept = 0;
  /*=======================
   *= Capacity Management =
   *=======================*/
  virtual size_t size() const noexcept = 0;
  // Clear the underlying storage, optionally reserving space (if supported by the underlying container).
  virtual void clear(size_t reserve = 0) = 0;
  // Implmenet
  virtual size_t find(bits::span<const u8> data) const noexcept = 0;
  virtual size_t strlen(size_t offset) const noexcept = 0;
  /*==================
   *= Input / Output =
   *==================*/
  // Insert iovec-style entries into the layout vector in-place, with a base offset of dst_offset.
  virtual size_t calculate_layout(std::vector<LayoutItem> &layout, size_t dst_offset) const = 0;

  // Helper to coerce an integeral types to spans
  template <std::integral I> void set(size_t offset, I value) {
    set(offset, bits::span<const u8>{(const u8 *)&value, sizeof(I)});
  }
  // Helper to coerce trivially-copyable types to spans. Must be non-integral / non-span to avoid angering compiler.
  template <typename T>
    requires(!std::is_integral_v<std::remove_cvref_t<T>> && !is_span_v<T>)
  void set(size_t offset, T &&data) {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    return set(offset, bits::span<const u8>((const u8 *)&data, sizeof(T)));
  }

  // Helper to coerce a non-span type into a span
  template <typename T>
    requires(!is_span_v<T>)
  size_t append(T &&data) {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    return append(bits::span<const u8>((const u8 *)&data, sizeof(T)));
  }
  // Helpers which access the block storage as a fixed-size array with type T entries.
  // Automatically computes an offset useing index and sizeof(T).
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
  // Helpers which allow access to an arbitrary offset into the storage as type T.
  // Useful for sections like .gnu_version_r where entries are variable-sized but at known locations.
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
  // Convenience method for string tables to convert an index into a C-string.
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
  pepp::bts::PagedAllocator<u8> _allocator{};
};

struct MemoryMapped : public AStorage {
  explicit MemoryMapped(std::shared_ptr<MappedFile::Slice>);

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
  std::shared_ptr<MappedFile::Slice> _slice = nullptr;
};

// Always contains 0 bytes of data and rejects all writes / appends.
// Useful for SHT_NOBITS and SHT_NULL section types.
struct NullStorage : public AStorage {
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
};
} // namespace pepp::bts
