#include "core/sim/memory/ram/sparse.hpp"
#include "core/sim/memory/errors.hpp"

Sparse::Sparse(Device::ID id, Configuration config) : Device(), _config(config), _id(id) {}

const Device::Configuration &Sparse::config() const { return _config.base; }

const Device::ID Sparse::id() const { return _id; }

Device::Type Sparse::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

u64 Sparse::features() const { return 0; }

void Sparse::set_buffer(Buffer *tb) { _tb = tb; }

const Buffer *Sparse::buffer() const { return _tb; }

bool Sparse::can_generate_traces() const { return true; }

void Sparse::trace(bool enabled) {
  if (_tb) _tb->trace(_id, enabled);
}

bool Sparse::traced() const { return _tb ? _tb->traced(_id) : false; }

AddressSpan Sparse::span() const { return _config.span; }

Target::Result Sparse::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  auto offset = address - span.lower();

  // TODO: emit a pure read to TB.
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (!(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal) && _tb)
    ;

  while (dest.size() > 0) {
    const auto page_addr = offset & ~SPARSE_PAGE_MASK;
    const auto page_offset = offset & SPARSE_PAGE_MASK;
    const auto len = std::min<u32>(dest.size(), SPARSE_PAGE_SIZE - page_offset);
    if (const auto it = _pages.find(page_addr); it != _pages.end()) {
      const auto &page = it->second;
      const auto src = bits::span<const u8>{page.data.data(), page.data.size()}.subspan(page_offset);
      assert(src.size() >= len);
      bits::memcpy(dest.first(len), src.first(len));
    } else {
      std::fill_n(dest.begin(), len, _config.fill);
    }

    offset += len;
    dest = dest.subspan(len);
  }

  return {};
}

Target::Result Sparse::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  auto offset = address - span.lower();

  // Record changes, even if the come from UI. Otherwise, step back fails.
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (op.type != Operation::Type::BufferInternal && _tb)
    ;

  while (src.size() > 0) {
    const auto page_addr = offset & ~SPARSE_PAGE_MASK;
    const auto page_offset = offset & SPARSE_PAGE_MASK;
    const auto len = std::min<u32>(src.size(), SPARSE_PAGE_SIZE - page_offset);
    // Search for a page. If it does not exist, allocate it.
    PageMeta *dst_page = nullptr;
    if (auto it = _pages.find(page_addr); it != _pages.end()) dst_page = &it->second;
    else dst_page = &(_pages[page_addr] = make_page());

    assert(dst_page != nullptr);
    auto dst = bits::span<u8>{dst_page->data.data(), dst_page->data.size()}.subspan(page_offset);
    assert(src.size() >= len);
    assert(dst.size() >= len);
    bits::memcpy(dst.first(len), src.first(len));
    offset += len;
    src = src.subspan(len);
  }

  return {};
}

void Sparse::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  _config.fill = fill;
  for (auto &[_, meta] : _pages) _free.push(meta);
  _pages.clear();
}

void Sparse::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  for (const auto &[addr, meta] : _pages) {
    auto dest_subspan = dest.subspan(addr - _config.span.lower(), meta.data.size());
    const auto src_subspan = bits::span<const u8>{meta.data.data(), meta.data.size()};
    bits::memcpy(dest_subspan, src_subspan);
  }
}

Sparse::PageMeta Sparse::make_page(bool init) {
  PageMeta ret;
  if (!_free.empty()) {
    ret = _free.top();
    _free.pop();
  } else {
    _data.emplace_back();
    ret = PageMeta{};
    ret.data = _data.back();
  }
  if (init) std::fill(ret.data.begin(), ret.data.end(), _config.fill);
  return ret;
}
