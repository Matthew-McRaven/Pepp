#include "core/sim/memory/ram/sparse.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

PagePool::PagePool(AddressSpan span, u8 fill) : _span(span), _fill(fill) {}

void PagePool::read(Address address, bits::span<u8> dest) const {
  auto offset = address - _span.lower();
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
      std::fill_n(dest.begin(), len, _fill);
    }

    offset += len;
    dest = dest.subspan(len);
  }
}

void PagePool::write(Address address, bits::span<const u8> src) {
  auto offset = address - _span.lower();
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
}

void PagePool::clear(u8 fill) {
  _fill = fill;
  for (auto &[_, meta] : _pages) _free.push(meta);
  _pages.clear();
}

void PagePool::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  for (const auto &[addr, meta] : _pages) {
    auto dest_subspan = dest.subspan(addr - _span.lower(), meta.data.size());
    const auto src_subspan = bits::span<const u8>{meta.data.data(), meta.data.size()};
    bits::memcpy(dest_subspan, src_subspan);
  }
}

PagePool::PageMeta PagePool::make_page(bool init) {
  PageMeta ret;
  if (!_free.empty()) {
    ret = _free.top();
    _free.pop();
  } else {
    _data.emplace_back();
    ret = PageMeta{};
    ret.data = _data.back();
  }
  if (init) std::fill(ret.data.begin(), ret.data.end(), _fill);
  return ret;
}

namespace {
Device *create_sparse(const nlohmann::json &self, System *sys, Device *par) {
  Sparse::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("RAM must have a basename");
    if (!self.contains("min_offset") || self["min_offset"].is_null()) throw ParsingError("RAM must have a min_offset");
    auto min = as_u32(self["min_offset"]);
    if (!self.contains("max_offset") || self["max_offset"].is_null()) throw ParsingError("RAM must have a max_offset");
    auto max = as_u32(self["max_offset"]);
    cfg.span = AddressSpan{min, max};
    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = as_i8(self["fill"]);
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse dense RAM: " + std::string(e.what()));
  }
  return sys->make_device<Sparse>(par, cfg);
}
void prefill_sparse(nlohmann::json &obj) {
  obj["compatible"] = Sparse::compatible;
  obj["basename"];
  obj["min_offset"];
  obj["max_offset"];
  obj["fill"] = 0;
}
void serialize_sparse(nlohmann::json &obj, const System *sys, const Device *self) {
  throw std::logic_error("Sparse::serialize not implemented");
}
} // namespace

Sparse::Sparse(Configuration config) : Device(), _config(config), _pool(_config.span, _config.fill) {}

const Device::Configuration &Sparse::config() const { return _config; }

const Device::ID Sparse::id() const { return _config.id; }

Device::Type Sparse::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

u64 Sparse::features() const { return 0; }

std::unique_ptr<DeviceSerializer> Sparse::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> Sparse::make_serializer() {
  DeviceSerializer s{.parser = create_sparse,
                     .prefill = prefill_sparse,
                     .serialize = serialize_sparse,
                     .compatible = Sparse::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

void Sparse::set_buffer(Buffer *tb) { _tb = tb; }

const Buffer *Sparse::buffer() const { return _tb; }

bool Sparse::can_generate_traces() const { return true; }

void Sparse::trace(bool enabled) {
  if (_tb) _tb->trace(id(), enabled);
}

bool Sparse::traced() const { return _tb ? _tb->traced(id()) : false; }

AddressSpan Sparse::span() const { return _config.span; }

Target::Result Sparse::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);

  // TODO: emit a pure read to TB.
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (!(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal) && _tb)
    ;
  _pool.read(address, dest);
  return {};
}

Target::Result Sparse::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);

  // Record changes, even if the come from UI. Otherwise, step back fails.
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (op.type != Operation::Type::BufferInternal && _tb)
    ;
  _pool.write(address, src);
  return {};
}

void Sparse::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  _config.fill = fill;
  _pool.clear(fill);
}

void Sparse::dump(bits::span<u8> dest) const { _pool.dump(dest); }