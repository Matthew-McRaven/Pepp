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
#include <algorithm>
#include <memory>
#include <utility>
#include <vector>
#include "../bitmanip/span.hpp"
#include "bts/bitmanip/integers.h"
#include "bts/bitmanip/log2.hpp"

namespace pepp::bts {
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
template <std::unsigned_integral I> class PagedAllocator {
public:
  // An offset into Page::data.
  using page_offset_t = size_t;
  // An offset into the PagedAllocator::_pages vector.
  using page_index_t = size_t;
  // An offset into the global allocation space, which needs to be converted to a (page_index_t, page_offset_t) pair.
  using global_offset_t = size_t;
  struct PageIndices {
    page_index_t index = 0;
    page_offset_t offset = 0;
  };
  // Holds multiple allocations in a contiguous block, with capacity being a power-of-two.
  // Like a slab allocator, individual allocations cannot be freed. Deallocation is not currently supported, but would
  // imply invalidation of existing offsets higher than the deallocated one.
  struct Page {
    // Pages are contain unintialized data up to capacity. Capacity is rounded to nearest power-of-two.
    explicit Page(page_offset_t capacity);
    // Copy elements into next free space, advancing size.
    // Require data be aligned % align (padding at start) with pad (padding at end).
    // Both align and pad are in element counts, not bytes.
    page_offset_t append(bits::span<const I> data, size_t byte_align = 0, size_t byte_pad = 0, I fill = 0);
    // An append will all elements set to `fill`.
    page_offset_t allocate_initialized(size_t size, I fill = 0);
    // Bump size without modifying underlying data.
    page_offset_t allocate_uninitialized(size_t size);
    // Check if the requested size can fit in the remaining space.
    bool can_fit(bits::span<const I> request, size_t align = 0, size_t pad = 0) const noexcept;
    bool can_fit(size_t request) const noexcept;
    // Set size to 0 without writing to underlying data.
    void clear() noexcept;
    // Set underlying data without modifying siz, capacity.
    void fill(page_offset_t from, page_offset_t to, I fill) noexcept;

    page_offset_t size() const noexcept;
    page_offset_t capacity() const noexcept;
    page_offset_t remaining_capacity() const noexcept;
    I *data() noexcept;
    const I *data() const noexcept;
    // Compute the size of a proposed allocation at the nexr available slot, ensuring a given byte alignment and byte
    // padding at the end. Returns the size (in elements) needed to satisfy the request. May overalign/overpad to
    // to achieve an integral number of elements.
    // Align bytes should be power-of-two.
    size_t padded_size(size_t count, size_t align_bytes, size_t pad_bytes) const noexcept;

  private:
    page_offset_t _capacity = 0, _size = 0;
    // Automatically allocated on construction
    std::unique_ptr<I[]> _data = nullptr;
  };

  static constexpr size_t MIN_PAGE_SIZE = 256;      // Minimum size for a single page.
  static constexpr size_t DEFAULT_PAGE_SIZE = 4096; // Default allocation size for a single page.
  static constexpr size_t MAX_PAGE_SIZE = 65535;    // Maximum number of elements than can be stored in a single page.
  static_assert(MIN_PAGE_SIZE <= MAX_PAGE_SIZE, "");
  static_assert(DEFAULT_PAGE_SIZE <= MAX_PAGE_SIZE, "");

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
  Page &page(page_index_t index);
  const Page &page(page_index_t index) const;
  bits::span<Page const> pages() const noexcept;
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
  std::vector<Page> _pages = {};
  // Cached offsets for the start of each page, to speed up indices_for_offset calculations.
  // Any call which inserts in the middle must update this vector.
  // Sorted by definition, so binary_search / upper/lower bounds will work.
  std::vector<size_t> _page_base = {};
};
template <std::unsigned_integral I>
size_t PagedAllocator<I>::Page::padded_size(size_t count, size_t align_bytes, size_t pad_bytes) const noexcept {
  if (align_bytes > alignof(I)) {
    const std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(_data.get() + _size);
    const size_t mis = static_cast<size_t>(addr % align_bytes);
    if (mis) count += bits::ceil_div(align_bytes - mis, sizeof(I));
  }
  return count + bits::ceil_div(pad_bytes, sizeof(I));
}

template <std::unsigned_integral I>
PagedAllocator<I>::Page::Page(page_offset_t capacity)
    : _capacity(bits::nearest_power_of_two(capacity)), _data(new I[_capacity]) {
  if (_capacity < MIN_PAGE_SIZE) throw std::invalid_argument("Allocation smaller than MIN_PAGE_SIZE");
  else if (_capacity > MAX_PAGE_SIZE) throw std::invalid_argument("Allocation larger than MAX_PAGE_SIZE");
}

template <std::unsigned_integral I>
PagedAllocator<I>::page_offset_t PagedAllocator<I>::Page::append(bits::span<const I> data, size_t align, size_t pad,
                                                                 I fill) {
  static_assert(std::is_trivially_copyable_v<I>);
  const auto total_size = this->padded_size(data.size(), align, pad);
  const auto padded_base = this->padded_size(_size, align, 0);
  if (total_size > remaining_capacity()) throw std::runtime_error("Page overflow");
  this->fill(_size, padded_base, fill);
  std::copy(data.begin(), data.end(), _data.get() + padded_base);
  this->fill(padded_base + data.size(), _size + total_size, fill);
  _size += total_size;
  return padded_base; // Return aligned pointer
}

template <std::unsigned_integral I>
PagedAllocator<I>::page_offset_t PagedAllocator<I>::Page::allocate_initialized(size_t size, I v) {
  if (size > remaining_capacity()) throw std::runtime_error("Page overflow");
  fill(_size, _size + size, v);
  return std::exchange(_size, _size + size);
}

template <std::unsigned_integral I>
PagedAllocator<I>::page_offset_t PagedAllocator<I>::Page::allocate_uninitialized(size_t size) {
  if (size > remaining_capacity()) throw std::runtime_error("Page overflow");
  return std::exchange(_size, _size + size);
}

template <std::unsigned_integral I> inline bool PagedAllocator<I>::Page::can_fit(size_t request) const noexcept {
  return request <= remaining_capacity();
}

template <std::unsigned_integral I>
bool PagedAllocator<I>::Page::can_fit(bits::span<const I> request, size_t align, size_t pad) const noexcept {
  return padded_size(request.size(), align, pad) <= remaining_capacity();
}

template <std::unsigned_integral I> void PagedAllocator<I>::Page::clear() noexcept { _size = 0; }

template <std::unsigned_integral I>
void PagedAllocator<I>::Page::fill(page_offset_t from, page_offset_t to, I fill) noexcept {
  from = std::min(from, _capacity), to = std::min(to, _capacity);
  if (from > to) std::swap(from, to);
  std::fill(_data.get() + from, _data.get() + to, fill);
}

template <std::unsigned_integral I>
typename PagedAllocator<I>::page_offset_t PagedAllocator<I>::Page::remaining_capacity() const noexcept {
  return _capacity - _size;
}

template <std::unsigned_integral I>
typename PagedAllocator<I>::page_offset_t PagedAllocator<I>::Page::capacity() const noexcept {
  return _capacity;
}

template <std::unsigned_integral I>
typename PagedAllocator<I>::page_offset_t PagedAllocator<I>::Page::size() const noexcept {
  return _size;
}

template <std::unsigned_integral I> I *PagedAllocator<I>::Page::data() noexcept { return _data.get(); }

template <std::unsigned_integral I> const I *PagedAllocator<I>::Page::data() const noexcept { return _data.get(); }

template <std::unsigned_integral I>
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

template <std::unsigned_integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::offset_for_indices(PageIndices indices) const {
  return _page_base.at(indices.index) + indices.offset;
}

template <std::unsigned_integral I> PagedAllocator<I>::Page &PagedAllocator<I>::page(page_index_t index) {
  return _pages[index];
}

template <std::unsigned_integral I> const PagedAllocator<I>::Page &PagedAllocator<I>::page(page_index_t index) const {
  return _pages[index];
}

template <std::unsigned_integral I>
bits::span<const typename PagedAllocator<I>::Page> PagedAllocator<I>::pages() const noexcept {
  return bits::span<const typename PagedAllocator<I>::Page>{_pages.data(), _pages.size()};
}

template <std::unsigned_integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::append(bits::span<const I> data, size_t align, size_t pad,
                                                             I fill) {
  if (!_pages.empty() && _pages.back().can_fit(data, align, pad)) {
    size_t old_size = _pages.back().size();
    _pages.back().append(data, align, pad, fill);
    return std::exchange(_size, _size + _pages.back().size() - old_size);
  } else {
    _pages.emplace_back(std::max<size_t>(DEFAULT_PAGE_SIZE, data.size())), _page_base.emplace_back(_size);
    _pages.back().append(data, align, pad, fill);
    return std::exchange(_size, _size + _pages.back().size());
  }
}

template <std::unsigned_integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::allocate_initialized(global_offset_t size, I fill) {
  if (!_pages.empty() && _pages.back().can_fit(size)) _pages.back().allocate_initialized(size, fill);
  else {
    _pages.emplace_back(std::max<size_t>(DEFAULT_PAGE_SIZE, size)), _page_base.emplace_back(_size);
    _pages.back().allocate_initialized(size, fill);
  }
  return std::exchange(_size, _size + size);
}

template <std::unsigned_integral I>
PagedAllocator<I>::global_offset_t PagedAllocator<I>::allocate_uninitialized(global_offset_t size) {
  if (!_pages.empty() && _pages.back().can_fit(size)) _pages.back().allocate_uninitialized(size);
  else {
    _pages.emplace_back(std::max<size_t>(DEFAULT_PAGE_SIZE, size)), _page_base.emplace_back(_size);
    _pages.back().allocate_uninitialized(size);
  }
  return std::exchange(_size, _size + size);
}

template <std::unsigned_integral I>
PagedAllocator<I>::InsertResult PagedAllocator<I>::insert(bits::span<const I> data, size_t align, size_t pad, I fill) {
  // Walk the pages until we find one that can fit the data.
  // Keep _page_base in sync with
  for (size_t it = 0; it < _pages.size(); it++)
    if (auto &page = _pages[it]; page.can_fit(data, align, pad)) {
      auto padded_size = page.padded_size(data.size(), align, pad);
      auto inserted_offset = page.append(data, align, pad, fill);
      // Insert causes _page_base beyond this page to shift forward by allocation size
      for (size_t jt = it + 1; jt < _pages.size(); jt++) _page_base[jt] += data.size();
      _size += padded_size;
      return InsertResult{.adjust_above = _page_base[it] + inserted_offset,
                          .adjust_by = padded_size,
                          .indices = {.index = it, .offset = inserted_offset}};
    }
  const auto adjust_above = append(data, align, pad, fill);
  const page_offset_t page_offset = adjust_above - _page_base.back();
  return {.adjust_above = adjust_above, .adjust_by = 0, .indices = {.index = _pages.size() - 1, .offset = page_offset}};
}

template <std::unsigned_integral I>
bits::span<I> PagedAllocator<I>::get(global_offset_t offset, size_t length) noexcept {
  if (offset + length > _size) return {};
  auto [page_index, page_offset] = indices_for_offset(offset);
  if (auto &page = _pages[page_index]; page_offset + length > page.size()) return {};
  else return bits::span<I>(page.data() + page_offset, length);
}

template <std::unsigned_integral I>
bits::span<const I> PagedAllocator<I>::get(global_offset_t offset, size_t length) const noexcept {
  if (offset + length > _size) return {};
  auto [page_index, page_offset] = indices_for_offset(offset);
  if (auto &page = _pages[page_index]; page_offset + length > page.size()) return {};
  else return bits::span<const I>(page.data() + page_offset, length);
}

template <std::unsigned_integral I> void PagedAllocator<I>::clear() noexcept {
  _size = 0, _page_base.clear(), _pages.clear();
}

template <std::unsigned_integral I> PagedAllocator<I>::global_offset_t PagedAllocator<I>::size() const noexcept {
  return _size;
}

template <std::unsigned_integral I> PagedAllocator<I>::global_offset_t PagedAllocator<I>::page_count() const noexcept {
  return _pages.size();
}

} // namespace pepp::bts
