/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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
#include <list>
#include <stack>
#include <unordered_map>
#include "core/sim/api/device.hpp"
#include "core/sim/api/memory.hpp"
#include "core/sim/api/trace.hpp"

// In the future, consider combining with core/ds/alloc/paged.hpp.
// I cannot combine them today, because PagedAllocator is not sparse.
// e.g., the page at 0x1'0000'0000 will not be allocated until all preceding page are allocated.
// This PagePool is intentionally spare because it is trying to store a large address space with the minimum number of
// bytes.
// It has no upper or lower bounds, and pages will be lazily allocated on demand.
class PagePool {
public:
  explicit PagePool(u8 fill = 0);
  ~PagePool() = default;
  PagePool(PagePool &&other) noexcept = default;
  PagePool &operator=(PagePool &&other) = default;
  PagePool(const PagePool &) = delete;
  PagePool &operator=(const PagePool &) = delete;

  void read(Address offset, bits::span<u8> dest) const;
  void write(Address offset, bits::span<const u8> src);
  void clear(u8 fill);
  void dump(bits::span<u8> dest) const;

private:
  u8 _fill;
  static constexpr u32 SPARSE_PAGE_SIZE = 256;
  static constexpr u32 SPARSE_PAGE_MASK = SPARSE_PAGE_SIZE - 1;
  using PageData = std::array<u8, SPARSE_PAGE_SIZE>;
  // I plan on adding cow/shared pages. Someday each "slot" in _pages will point to a PageMeta (rather than by value).
  // With the by-value approach, I can't have COW semantics.
  struct PageMeta {
    PageData data;
  };
  // Return a new PageMeta which points to an unused data page.
  // Prefferentially pull from _free, otherwise allocate a new data page.
  // Initialize all values in page to _fill if true, otherwise returned array as-is.
  PageMeta make_page(bool init = true);
  std::unordered_map<Address, PageMeta> _pages;
  std::stack<PageMeta> _free;
  std::list<PageData> _data;
};

class Sparse final : public Device, public Target, public Traceable {
public:
  static const inline std::string compatible = "ram,sparse";
  struct Configuration : public Device::Configuration {
    u8 fill = 0;
    AddressSpan span{};
  };
  Sparse(Configuration config);
  ~Sparse() = default;
  Sparse(Sparse &&other) noexcept = default;
  Sparse &operator=(Sparse &&other) = default;
  // Disable copy construction and assignment, since it would be incorrect for
  // multiple objects to share a device descriptor.
  Sparse(const Sparse &) = delete;
  Sparse &operator=(const Sparse &) = delete;

  // Device interface
  const Device::Configuration &config() const override;
  const Device::ID id() const override;
  Device::Type type() const override;
  u64 features() const override;
  std::unique_ptr<DeviceSerializer> serializer() const override;
  static std::unique_ptr<DeviceSerializer> make_serializer();

  // TraceSource interface
  void set_buffer(Buffer *tb) override;
  const Buffer *buffer() const override;
  bool can_generate_traces() const override;
  void trace(bool enabled) override;
  bool traced() const override;

  // Target interface
  AddressSpan span() const override;
  Result read(Address address, bits::span<u8> dest, Operation op) const override;
  Result write(Address address, bits::span<const u8> src, Operation op) override;
  void clear(u8 fill) override;
  void dump(bits::span<u8> dest) const override;

private:
  Configuration _config;
  PagePool _pool;
  Buffer *_tb = nullptr;
};
