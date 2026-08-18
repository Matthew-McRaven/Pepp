#include "core/sim/memory/ram/sparse.hpp"
#include <nlohmann/json.hpp>
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

void dump(const pepp::bts::PagedPool<u8> &p, bits::span<u8> dest) {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  for (const auto &[addr, pg] : p.pages()) {
    auto dest_subspan = dest.subspan(addr, pg.capacity());
    const auto src_subspan = bits::span<const u8>{pg.data(), pg.capacity()};
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
    throw ParsingError("Failed to parse sparse RAM: " + std::string(e.what()));
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
  auto casted = dynamic_cast<const Sparse *>(self);
  if (!casted) throw std::logic_error("serialize_sparse called on non-Sparse device");
  obj["compatible"] = Sparse::compatible;
  obj["basename"] = casted->config().basename;
  obj["min_offset"] = casted->casted_config().span.lower();
  obj["max_offset"] = casted->casted_config().span.upper();
  if (casted->casted_config().fill != 0) obj["fill"] = casted->casted_config().fill;
}
} // namespace

Sparse::Sparse(Configuration config) : Device(), _config(config), _pool(_config.fill) {}

const Device::Configuration &Sparse::config() const { return _config; }

const Sparse::Configuration &Sparse::casted_config() const { return _config; }

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

void Sparse::set_recorder(const trace::Recorder &recorder) { _trace = recorder; }

bool Sparse::can_generate_traces() const { return true; }

void Sparse::trace(bool enabled) { _trace.set_traced(enabled); }

bool Sparse::traced() const { return _trace.traced(); }

AddressSpan Sparse::span() const { return _config.span; }

Target::Result Sparse::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);

  const auto offset = address - span.lower();
  _pool.read(offset, dest);
  if (is_performance_countable(op)) _counters.rd_bytes += dest.size();
  return {};
}

Target::Result Sparse::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);

  const auto offset = address - span.lower();
  // Unlike dense, we don't have a convenient span of the previous bytes, because we might not be accessing a flat
  // array. Instead, we use the "filler" form, which will copy the bytes out of the pool into the trace buffer before we
  // ^ in place. The callback is only executed IF the write is recorded. We still pay the price of alloc'ing a lambda,
  // but we don't pay the cost of reading the data.
  _trace.emit_write(op, address, src, [&](bits::span<u8> prior) { _pool.read(offset, prior); });
  _pool.write(offset, src);
  if (is_performance_countable(op)) _counters.wr_bytes += src.size();
  return {};
}

void Sparse::clear(u8 fill) {
  _config.fill = fill;
  _pool.clear(fill);
}

void Sparse::dump(bits::span<u8> dest) const { ::dump(_pool, dest); }