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
  virtual u64 append(bits::span<const u8> data) = 0;
  virtual u64 allocate(u64 size,
                       u8 fill = 0) = 0; // Allocates `size` uninitialized contiguous bytes and returns the offset.
  // Overwrites data at the given offset, raising an exception if offset+data.size() exceeds current storage.
  virtual void set(u64 offset, bits::span<const u8> data) = 0;
  // Retrieves a span representing the data at the given offset and length. Return nulltpr if offset+length exceeds
  // current storage.
  virtual bits::span<u8> get(u64 offset, u64 length) noexcept = 0;
  virtual bits::span<const u8> get(u64 offset, u64 length) const noexcept = 0;
  virtual u64 size() const noexcept = 0;
  // Clear the underlying storage and attempt to reserve a number of bytes.
  virtual void clear(u64 reserve = 0) = 0;
  // Returns the computed final offset
  virtual u64 calculate_layout(std::vector<LayoutItem> &layout, u32 dst_offset) const = 0;
  virtual u64 find(bits::span<const u8> data) const noexcept = 0;
  virtual u64 strlen(u64 offset) const noexcept = 0;

  // Helpers
  template <std::integral I> void set(u64 offset, I value) {
    set(offset, bits::span<const u8>{(const u8 *)&value, sizeof(I)});
  }
  // Do not enable for integral
  template <typename T>
    requires(!std::is_integral_v<std::remove_cvref_t<T>> && !is_span_v<T>)
  void set(u64 offset, T &&data) {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    return set(offset, bits::span<const u8>((const u8 *)&data, sizeof(T)));
  }

  template <typename T>
    requires(!is_span_v<T>)
  u64 append(T &&data) {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    return append(bits::span<const u8>((const u8 *)&data, sizeof(T)));
  }
  // Assuming the section contains fixed-size entries of type T, get the entry at the given index.
  template <typename T> const T *get(u64 index) const noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(sizeof(T) * index, sizeof(T));
    if (span.size() != sizeof(T)) return nullptr;
    return reinterpret_cast<const T *>(span.data());
  }
  template <typename T> T *get(u64 index) noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(sizeof(T) * index, sizeof(T));
    if (span.size() != sizeof(T)) return nullptr;
    return reinterpret_cast<T *>(span.data());
  }
  // You have an offset into the storage, but your items are not fixed-size so you cannot use get<T>(index)
  template <typename T> const T *get_at(u64 offset) const noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(offset, sizeof(T));
    return reinterpret_cast<const T *>(span.data());
  }
  template <typename T> T *get_at(u64 offset) noexcept {
    static_assert(std::is_trivially_copyable_v<std::remove_cvref_t<T>>, "Requires trivially copyable types");
    static_assert(alignof(T) == 1, "Types must be packed / have alignment of 1");
    auto span = get(offset, sizeof(T));
    return reinterpret_cast<T *>(span.data());
  }
  inline bits::span<const u8> get_string_span(u64 index) const noexcept { return get(index, strlen(index)); }
};

// Vector-backed storage
struct BlockStorage : public AStorage {
  // AStorage interface
  u64 append(bits::span<const u8> data) override;
  u64 allocate(u64 size, u8 fill = 0) override;
  void set(u64 offset, bits::span<const u8> data) override;
  bits::span<u8> get(u64 offset, u64 length) noexcept override;
  bits::span<const u8> get(u64 offset, u64 length) const noexcept override;
  u64 size() const noexcept override;
  void clear(u64 reserve = 0) override;
  u64 calculate_layout(std::vector<LayoutItem> &layout, u32 dst_offset) const override;
  u64 find(bits::span<const u8> data) const noexcept override;
  u64 strlen(u64 offset) const noexcept override;

private:
  std::vector<char> _storage{};
};
} // namespace pepp::bts
