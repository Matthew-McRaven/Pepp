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

#pragma once
#include <algorithm>
#include <memory>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>
#include "core/math/bitmanip/copy.hpp"
#include "core/math/bitmanip/log2.hpp"
#include "core/math/bitmanip/span.hpp"

namespace pepp::bts {

// A class which represents a power-of-2-sized block of contiguous memory.
// PagedPool uses this primitive to construct a lazyily-allocated sparse array of memory,
// while PagedAllocator (and Slab) add allocation APIs on top of this primitive.
// TODO: add copy-on-write, sharing semantics
template <std::integral I> class Page {
public:
  static constexpr size_t MIN_PAGE_SIZE = 256;      // Minimum size for a single page.
  static constexpr size_t DEFAULT_PAGE_SIZE = 4096; // Default allocation size for a single page.
  static constexpr size_t MAX_PAGE_SIZE = 65536;    // Maximum number of elements than can be stored in a single page.
  static_assert(MIN_PAGE_SIZE <= MAX_PAGE_SIZE, "");
  static_assert(DEFAULT_PAGE_SIZE <= MAX_PAGE_SIZE, "");

  // An offset into Page::data.
  using page_offset_t = size_t;

  // Pages are contain unintialized data up to capacity. Capacity is rounded to nearest power-of-two.
  explicit Page(page_offset_t capacity);
  // Don't allow implicit copy, because this class owns an expensive resource.
  Page(const Page &) = delete;
  Page &operator=(const Page &) = delete;
  Page(Page &&) noexcept = default;
  Page &operator=(Page &&) noexcept = default;

  // Set underlying data without modifying siz, capacity.
  void fill(page_offset_t from, page_offset_t to, I fill) noexcept;
  void fill(I fill) noexcept;
  page_offset_t capacity() const noexcept;
  I *data() noexcept;
  const I *data() const noexcept;
  std::span<I> span() noexcept { return std::span<I>(_data.get(), _capacity); }
  std::span<const I> span() const noexcept { return std::span<const I>(_data.get(), _capacity); }

private:
  page_offset_t _capacity = 0;
  // Allocated on construction.
  std::unique_ptr<I[]> _data = nullptr;
};

// A class meant for representing sparse data, like a the RAM of a 32-bit system.
// It has no upper-bound on size, and pages are lazily allocated.
template <std::integral I> class PagedPool {
public:
  explicit PagedPool(u8 fill = 0);
  ~PagedPool() = default;
  PagedPool(const PagedPool &) = delete;
  PagedPool &operator=(const PagedPool &) = delete;
  PagedPool(PagedPool &&) noexcept = default;
  PagedPool &operator=(PagedPool &&) noexcept = default;

  void read(size_t offset, bits::span<I> dest) const;
  void write(size_t offset, bits::span<const I> src);
  void clear(I fill);
  const auto &pages() const noexcept { return _pages; }

private:
  // Capacity and mask are hardcoded for now.
  static constexpr size_t SPARSE_PAGE_SIZE = Page<I>::DEFAULT_PAGE_SIZE;
  static constexpr u32 SPARSE_PAGE_MASK = SPARSE_PAGE_SIZE - 1;

  // Return a new Page which points to an unused data page.
  // Preferentially pull from _free, otherwise allocate a new data page.
  // Initialize all values in page to _fill if true, otherwise returned array as-is.
  Page<I> make_page(bool init = false);

  I _fill = 0;
  std::stack<Page<I>> _free;
  std::unordered_map<size_t, Page<I>> _pages;
};

// Holds multiple allocations in a contiguous block, with capacity being a power-of-two.
// Like a slab allocator, individual allocations cannot be freed. Deallocation is not currently supported, but would
// imply invalidation of existing offsets higher than the deallocated one.
// Adds an allocation API on top of Page, and tracks how many bytes are allocated within this page.
template <std::integral I> struct Slab : public Page<I> {
  using page_offset_t = typename Page<I>::page_offset_t;
  // Pages are contain unintialized data up to capacity. Capacity is rounded to nearest power-of-two.
  explicit Slab(page_offset_t capacity);
  // Don't allow implicit copy, because this class owns an expensive resource.
  Slab(const Slab &) = delete;
  Slab &operator=(const Slab &) = delete;
  Slab(Slab &&) noexcept = default;
  Slab &operator=(Slab &&) noexcept = default;

  // Set size to 0 without writing to underlying data.
  void clear() noexcept;
  // Set all elements to value before calling clear().
  void fill_clear(I value) noexcept;

  // Copy elements into next free space, advancing size.
  // Require data be aligned % align (padding at start) with pad (padding at end).
  // Both align and pad are in element counts, not bytes.
  page_offset_t append(bits::span<const I> data, size_t byte_align = 0, size_t byte_pad = 0, I fill = 0);
  // Alternate overload to append which takes many spans (at the end) all with the same align/pad/fill requirements.
  template <typename... Spans> page_offset_t append(size_t byte_align, size_t byte_pad, I fill, Spans... spans);
  // Append a series of spans with no alignment or padding.
  template <typename... Spans> page_offset_t append_packed(Spans... spans);
  // An append will all elements set to `fill`.
  page_offset_t allocate_initialized(size_t size, I fill = 0);
  // Bump size without modifying underlying data.
  page_offset_t allocate_uninitialized(size_t size);

  // Check if the requested size can fit in the remaining space.
  bool can_fit(bits::span<const I> request, size_t align = 0, size_t pad = 0) const noexcept;
  bool can_fit(size_t request) const noexcept;
  page_offset_t used_capacity() const noexcept;
  page_offset_t remaining_capacity() const noexcept;
  // Compute the size of a proposed allocation at the nexr available slot, ensuring a given byte alignment and byte
  // padding at the end. Returns the size (in elements) needed to satisfy the request. May overalign/overpad to
  // to achieve an integral number of elements.
  // Align bytes should be power-of-two.
  size_t padded_size(size_t count, size_t align_bytes, size_t pad_bytes) const noexcept;
  // As padded_size, but for an allocation placed at `at` rather than at the next free slot. Lets a run
  // of appends be measured before any of it is written, since each one's alignment depends on where
  // the previous one left off.
  size_t padded_size_at(size_t at, size_t count, size_t align_bytes, size_t pad_bytes) const noexcept;
  // Whether a whole collection of spans fits, given that append() pads and aligns each one individually.
  template <typename... Spans> bool can_fit_all(size_t byte_align, size_t byte_pad, Spans... spans) const noexcept {
    size_t at = _used;
    ((at += padded_size_at(at, spans.size(), byte_align, byte_pad)), ...);
    return (at - _used) <= remaining_capacity();
  }

private:
  page_offset_t _used = 0;
};

// A class meant to allocate large numbers of small objects efficiently like  strings in a .shstrab section of an ELF
// file. It is effectively append-only in that you cannot delete elements in the middle, nor can you perform arbitrary
// insertion.
//
// global_offset_t provides an abstraction that this class acts as-if an std::vector<I> despite underlying memory
// fragmentation (i.e., paging). The class guarantees that allocations made via append/allocate_* are contiguous.
// That is, if you allocate a trivially copyable C++ object with the correct alignment via append, you can cast
// back to the original type from get. Any attempts to form cross-page spans will result in either an empty span or an
// exception.
//
// Pages are allocated on-demand and never shrunk or freed. Only the page's prefix (offsets [0, size()]) are considered
// valid data. Only the last page is considered a target for append/allocate_*, which can lead to unused suffix in
// pages. insert() uses a first-fit strategy to find space for data in any page's suffix, at the cost of invalidating
// existing global offsets into any page beyond the insertion point.
template <std::integral I> class PagedAllocator {
public:
  // An offset into the PagedAllocator::_pages vector.
  using page_index_t = size_t;
  using page_offset_t = typename Page<I>::page_offset_t;
  // An offset into the global allocation space, which needs to be converted to a (page_index_t, page_offset_t) pair.
  using global_offset_t = size_t;
  struct PageIndices {
    page_index_t index = 0;
    page_offset_t offset = 0;
  };

  // Access underlying pages in the allocator.

  PagedAllocator() = default;
  ~PagedAllocator() = default;
  PagedAllocator(const PagedAllocator &) = delete;
  PagedAllocator &operator=(const PagedAllocator &) = delete;
  PagedAllocator(PagedAllocator &&) noexcept = default;
  PagedAllocator &operator=(PagedAllocator &&) noexcept = default;

  /*===============
   *= Page Access =
   *===============*/
  PageIndices indices_for_offset(global_offset_t offset) const;
  global_offset_t offset_for_indices(PageIndices indices) const;
  Slab<I> &page(page_index_t index);
  const Slab<I> &page(page_index_t index) const;
  bits::span<Slab<I> const> pages() const noexcept;
  auto pages_cbegin() const noexcept { return _pages.cbegin(); }
  auto pages_cend() const noexcept { return _pages.cend(); }
  /*======================
   *= Element Creation   =
   *======================*/
  // Guarantees data will be allocated contiguously, even if it it larger than DEFAULT_PAGE_SIZE.
  // Always inserts at the end of the last page, allocating a new page if the current last page has insufficient
  // remaining capacity.
  // Require data be aligned % align (padding at start) with pad (padding at end).
  // Both align and pad are in element counts, not bytes.
  global_offset_t append(bits::span<const I> data, size_t byte_align = 0, size_t byte_pad = 0, I fill = 0);
  // Like append, but allocates a sequence of elements initialized to `fill`.
  global_offset_t allocate_initialized(global_offset_t size, I fill = 0);
  // Like allocate_initialized, but returns uninitialized memory.
  global_offset_t allocate_uninitialized(global_offset_t size);
  struct InsertResult {
    // Any global offset >= adjust_above must be incremented to remain valid.
    // All existing PageIndices remain valid.
    global_offset_t adjust_above = 0, adjust_by = 0;
    PageIndices indices;
  };
  // WARNING: may invalidate existing global offsets. If you have variables holding global offsets, you must update them
  // based on the return value. Insert data in the first available space that can fit it, even if that is not the last
  // page. Existing global offsets higher than the return value are invalidated.
  //
  InsertResult insert(bits::span<const I> data, size_t align = 0, size_t pad = 0, I fill = 0);
  /*======================
   *= Element Read/Write =
   *======================*/
  // Requires that (offset, offset+length) is contained within a single page.
  // Returns an empty span when request crosses multiple pages.
  bits::span<I> get(global_offset_t offset, size_t length) noexcept;
  bits::span<const I> get(global_offset_t offset, size_t length) const noexcept;
  /*=======================
   *= Capacity Management =
   *=======================*/
  void clear() noexcept;
  global_offset_t size() const noexcept;
  size_t page_count() const noexcept;

private:
  // Number of elements currently allocated.
  size_t _size = 0;
  std::vector<Slab<I>> _pages = {};
  // Cached offsets for the start of each page, to speed up indices_for_offset calculations.
  // Any call which inserts in the middle must update this vector.
  // Sorted by definition, so binary_search / upper/lower bounds will work.
  std::vector<size_t> _page_base = {};
};

/*
 * Page
 */
template <std::integral I>
Page<I>::Page(page_offset_t capacity) : _capacity(bits::nearest_power_of_two(capacity)), _data(new I[_capacity]) {
  if (_capacity < MIN_PAGE_SIZE) throw std::invalid_argument("Allocation smaller than MIN_PAGE_SIZE");
  else if (_capacity > MAX_PAGE_SIZE) throw std::invalid_argument("Allocation larger than MAX_PAGE_SIZE");
}

template <std::integral I> void Page<I>::fill(page_offset_t from, page_offset_t to, I fill) noexcept {
  from = std::min(from, _capacity), to = std::min(to, _capacity);
  if (from > to) std::swap(from, to);
  std::fill(_data.get() + from, _data.get() + to, fill);
}

template <std::integral I> inline void Page<I>::fill(I fill) noexcept { fill(0, _capacity, fill); }

template <std::integral I> I *Page<I>::data() noexcept { return _data.get(); }

template <std::integral I> const I *Page<I>::data() const noexcept { return _data.get(); }

template <std::integral I> typename Page<I>::page_offset_t Page<I>::capacity() const noexcept { return _capacity; }

/*
 * Slab
 */
template <std::integral I> Slab<I>::Slab(page_offset_t capacity) : Page<I>(capacity), _used(0) {}

template <std::integral I> void Slab<I>::clear() noexcept { _used = 0; }

template <std::integral I> void Slab<I>::fill_clear(I value) noexcept {
  // Fill from start to our high water mark.
  this->fill(0, _used, value);
  clear();
}

template <std::integral I>
Slab<I>::page_offset_t Slab<I>::append(bits::span<const I> data, size_t align, size_t pad, I fill) {
  static_assert(std::is_trivially_copyable_v<I>);
  const auto total_size = this->padded_size(data.size(), align, pad);
  const auto padded_base = this->padded_size(_used, align, 0);
  if (total_size > remaining_capacity()) throw std::runtime_error("Page overflow");
  this->fill(_used, padded_base, fill);
  std::copy(data.begin(), data.end(), this->data() + padded_base);
  this->fill(padded_base + data.size(), _used + total_size, fill);
  _used += total_size;
  return padded_base; // Return aligned pointer
}

template <std::integral I>
template <typename... Spans>
Slab<I>::page_offset_t Slab<I>::append(size_t byte_align, size_t byte_pad, I fill, Spans... spans) {
  // Measure the whole run before writing any of it.
  if (!can_fit_all(byte_align, byte_pad, spans...)) throw std::runtime_error("Page overflow");
  page_offset_t first{};
  bool is_first = true;
  ((is_first ? (first = append(spans, byte_align, byte_pad, fill), is_first = false)
             : (append(spans, byte_align, byte_pad, fill), false)),
   ...);
  return first;
}

template <std::integral I> template <typename... Spans> Slab<I>::page_offset_t Slab<I>::append_packed(Spans... spans) {
  return append(size_t{0}, size_t{0}, I{0}, spans...);
}

template <std::integral I> Slab<I>::page_offset_t Slab<I>::allocate_initialized(size_t size, I v) {
  if (size > remaining_capacity()) throw std::runtime_error("Page overflow");
  this->fill(_used, _used + size, v);
  return std::exchange(_used, _used + size);
}

template <std::integral I> Slab<I>::page_offset_t Slab<I>::allocate_uninitialized(size_t size) {
  if (size > remaining_capacity()) throw std::runtime_error("Page overflow");
  return std::exchange(_used, _used + size);
}

template <std::integral I> bool Slab<I>::can_fit(size_t request) const noexcept {
  return request <= remaining_capacity();
}

template <std::integral I> bool Slab<I>::can_fit(bits::span<const I> request, size_t align, size_t pad) const noexcept {
  return padded_size(request.size(), align, pad) <= remaining_capacity();
}

template <std::integral I> typename Slab<I>::page_offset_t Slab<I>::used_capacity() const noexcept { return _used; }

template <std::integral I> typename Slab<I>::page_offset_t Slab<I>::remaining_capacity() const noexcept {
  return Page<I>::capacity() - _used;
}

template <std::integral I>
size_t Slab<I>::padded_size(size_t count, size_t align_bytes, size_t pad_bytes) const noexcept {
  return padded_size_at(_used, count, align_bytes, pad_bytes);
}

template <std::integral I>
size_t Slab<I>::padded_size_at(size_t at, size_t count, size_t align_bytes, size_t pad_bytes) const noexcept {
  if (align_bytes > alignof(I)) {
    const std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(this->data() + at);
    const size_t mis = static_cast<size_t>(addr % align_bytes);
    if (mis) count += bits::ceil_div(align_bytes - mis, sizeof(I));
  }
  return count + bits::ceil_div(pad_bytes, sizeof(I));
}
/*
 * PagedPool
 */

template <std::integral I> inline PagedPool<I>::PagedPool(u8 fill) : _fill(fill) {}

template <std::integral I> void PagedPool<I>::read(size_t offset, bits::span<I> dest) const {
  while (dest.size() > 0) {
    const auto page_addr = offset & ~SPARSE_PAGE_MASK;
    const auto page_offset = offset & SPARSE_PAGE_MASK;
    const auto len = std::min<u32>(dest.size(), SPARSE_PAGE_SIZE - page_offset);
    if (const auto it = _pages.find(page_addr); it != _pages.end()) {
      const auto &page = it->second;
      const auto src = bits::span<const I>{page.data(), page.capacity()}.subspan(page_offset);
      // Switched on the width so the copy length is a constant the compiler can turn into a register operation for
      // common register sizes rather than a call to memcpy.
      switch (len) {
      case 1: std::memcpy(dest.data(), src.data(), 1); break;
      case 2: std::memcpy(dest.data(), src.data(), 2); break;
      case 4: std::memcpy(dest.data(), src.data(), 4); break;
      default: ::bits::memcpy(dest.first(len), src.first(len)); break;
      }
    } else {
      std::fill_n(dest.begin(), len, _fill);
    }

    offset += len;
    dest = dest.subspan(len);
  }
}

template <std::integral I> void PagedPool<I>::write(size_t offset, bits::span<const I> src) {
  while (src.size() > 0) {
    const auto page_addr = offset & ~SPARSE_PAGE_MASK;
    const auto page_offset = offset & SPARSE_PAGE_MASK;
    const auto len = std::min<u32>(src.size(), SPARSE_PAGE_SIZE - page_offset);
    // Search for a page. If it does not exist, allocate it.
    Page<I> *dst_page = nullptr;
    if (auto it = _pages.find(page_addr); it != _pages.end()) dst_page = &it->second;
    else {
      _pages.insert_or_assign(page_addr, make_page(true));
      dst_page = &(_pages.find(page_addr)->second);
    }

    auto dst = bits::span<u8>{dst_page->data(), dst_page->capacity()}.subspan(page_offset);
    // Switched on the width so the copy length is a constant the compiler can turn into a register operation for
    // common register sizes rather than a call to memcpy.
    switch (len) {
    case 1: std::memcpy(dst.data(), src.data(), 1); break;
    case 2: std::memcpy(dst.data(), src.data(), 2); break;
    case 4: std::memcpy(dst.data(), src.data(), 4); break;
    default: ::bits::memcpy(dst.first(len), src.first(len)); break;
    }

    offset += len;
    src = src.subspan(len);
  }
}

template <std::integral I> void PagedPool<I>::clear(I fill) {
  _fill = fill;
  for (auto &[_, meta] : _pages) _free.push(std::move(meta));
  _pages.clear();
}

template <std::integral I> Page<I> PagedPool<I>::make_page(bool init) {
  static const auto take = [](std::stack<Page<I>> &s) {
    Page<I> item = std::move(s.top());
    s.pop();
    return item;
  };
  Page<I> ret = (_free.empty() ? Page<I>(SPARSE_PAGE_SIZE) : take(_free));
  if (init) std::fill(ret.data(), ret.data() + ret.capacity(), _fill);
  return ret;
}

/*
 * PagedAllocator
 */
template <std::integral I>
PagedAllocator<I>::PageIndices PagedAllocator<I>::indices_for_offset(global_offset_t offset) const {
  if (offset >= _size) throw std::runtime_error("Invalid offset!!");
  if (_page_base.empty()) throw std::runtime_error("No pages!!");

  // it points to first base strictly greater than offset
  auto it = std::upper_bound(_page_base.begin(), _page_base.end(), offset);
  if (it == _page_base.begin()) throw std::runtime_error("Offset before first page!!");

  const page_index_t page_index = static_cast<page_index_t>(std::distance(_page_base.begin(), it) - 1);
  const global_offset_t page_base = _page_base[page_index];
  const page_offset_t page_offset = static_cast<page_offset_t>(offset - page_base);

  return PageIndices{page_index, page_offset};
}

template <std::integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::offset_for_indices(PageIndices indices) const {
  return _page_base.at(indices.index) + indices.offset;
}

template <std::integral I> Slab<I> &PagedAllocator<I>::page(page_index_t index) { return _pages[index]; }

template <std::integral I> const Slab<I> &PagedAllocator<I>::page(page_index_t index) const { return _pages[index]; }

template <std::integral I> bits::span<const Slab<I>> PagedAllocator<I>::pages() const noexcept {
  return bits::span<const Slab<I>>{_pages.data(), _pages.size()};
}

template <std::integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::append(bits::span<const I> data, size_t align, size_t pad,
                                                             I fill) {
  if (!_pages.empty() && _pages.back().can_fit(data, align, pad)) {
    size_t old_size = _pages.back().used_capacity();
    _pages.back().append(data, align, pad, fill);
    return std::exchange(_size, _size + _pages.back().used_capacity() - old_size);
  } else {
    _pages.emplace_back(std::max<size_t>(Slab<I>::DEFAULT_PAGE_SIZE, data.size())), _page_base.emplace_back(_size);
    _pages.back().append(data, align, pad, fill);
    return std::exchange(_size, _size + _pages.back().used_capacity());
  }
}

template <std::integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::allocate_initialized(global_offset_t size, I fill) {
  if (!_pages.empty() && _pages.back().can_fit(size)) _pages.back().allocate_initialized(size, fill);
  else {
    _pages.emplace_back(std::max<size_t>(Slab<I>::DEFAULT_PAGE_SIZE, size)), _page_base.emplace_back(_size);
    _pages.back().allocate_initialized(size, fill);
  }
  return std::exchange(_size, _size + size);
}

template <std::integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::allocate_uninitialized(global_offset_t size) {
  if (!_pages.empty() && _pages.back().can_fit(size)) _pages.back().allocate_uninitialized(size);
  else {
    _pages.emplace_back(std::max<size_t>(Slab<I>::DEFAULT_PAGE_SIZE, size)), _page_base.emplace_back(_size);
    _pages.back().allocate_uninitialized(size);
  }
  return std::exchange(_size, _size + size);
}

template <std::integral I>
PagedAllocator<I>::InsertResult PagedAllocator<I>::insert(bits::span<const I> data, size_t align, size_t pad, I fill) {
  // Walk the pages until we find one that can fit the data.
  for (size_t it = 0; it < _pages.size(); it++)
    if (auto &page = _pages[it]; page.can_fit(data, align, pad)) {
      auto padded_size = page.padded_size(data.size(), align, pad);
      auto inserted_offset = page.append(data, align, pad, fill);
      // Insert causes _page_base beyond this page to shift forward by the actual allocation size
      for (size_t jt = it + 1; jt < _pages.size(); jt++) _page_base[jt] += padded_size;
      _size += padded_size;
      return InsertResult{.adjust_above = _page_base[it] + inserted_offset,
                          .adjust_by = padded_size,
                          .indices = {.index = it, .offset = inserted_offset}};
    }
  const auto adjust_above = append(data, align, pad, fill);
  const page_offset_t page_offset = adjust_above - _page_base.back();
  return {.adjust_above = adjust_above, .adjust_by = 0, .indices = {.index = _pages.size() - 1, .offset = page_offset}};
}

template <std::integral I> bits::span<I> PagedAllocator<I>::get(global_offset_t offset, size_t length) noexcept {
  if (offset + length > _size) return {};
  auto [page_index, page_offset] = indices_for_offset(offset);
  if (auto &page = _pages[page_index]; page_offset + length > page.used_capacity()) return {};
  else return bits::span<I>(page.data() + page_offset, length);
}

template <std::integral I>
bits::span<const I> PagedAllocator<I>::get(global_offset_t offset, size_t length) const noexcept {
  if (offset + length > _size) return {};
  auto [page_index, page_offset] = indices_for_offset(offset);
  if (auto &page = _pages[page_index]; page_offset + length > page.used_capacity()) return {};
  else return bits::span<const I>(page.data() + page_offset, length);
}

template <std::integral I> void PagedAllocator<I>::clear() noexcept { _size = 0, _page_base.clear(), _pages.clear(); }

template <std::integral I> PagedAllocator<I>::global_offset_t PagedAllocator<I>::size() const noexcept { return _size; }

template <std::integral I> PagedAllocator<I>::global_offset_t PagedAllocator<I>::page_count() const noexcept {
  return _pages.size();
}

} // namespace pepp::core
