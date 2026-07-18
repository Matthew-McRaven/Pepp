#include "core/sim/memory/ram/sparse.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

void dump(const pepp::bts::PagedPool<u8> &p, bits::span<u8> dest) {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  for (const auto &[addr, pg] : p.pages()) {
    auto dest_subspan = dest.subspan(addr, pg.size());
    const auto src_subspan = bits::span<const u8>{pg.data(), pg.size()};
    bits::memcpy(dest_subspan, src_subspan);
  }
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

Sparse::Sparse(Configuration config) : Device(), _config(config), _pool(_config.fill) {}

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
  const auto offset = address - span.lower();
  _pool.read(offset, dest);
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
  const auto offset = address - span.lower();
  _pool.write(offset, src);
  return {};
}

void Sparse::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  _config.fill = fill;
  _pool.clear(fill);
}

void Sparse::dump(bits::span<u8> dest) const { ::dump(_pool, dest); }