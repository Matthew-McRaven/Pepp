#include "dense.hpp"
#include "core/sim/memory/errors.hpp"

Dense::Dense(Configuration config) : Device(), _config(config) {
  _data.resize(size_inclusive(*_config.span), *_config.fill);
}

std::span<const u8> Dense::data() const { return std::span<const u8>{_data.data(), std::size_t(_data.size())}; }

const Device::ID Dense::id() const { return *_config.id; }

const Device::Configuration &Dense::config() const { return _config; }

Device::Type Dense::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

u64 Dense::features() const { return 0; }

void Dense::set_buffer(Buffer *tb) { _tb = tb; }

const Buffer *Dense::buffer() const { return _tb; }

bool Dense::can_generate_traces() const { return true; }

void Dense::trace(bool enabled) {
  if (_tb) _tb->trace(id(), enabled);
}

bool Dense::traced() const { return _tb ? _tb->traced(id()) : false; }

AddressSpan Dense::span() const { return *_config.span; }

Target::Result Dense::read(Address address, bits::span<u8> dest, Operation op) const {
  using E = Error;
  const auto span = *_config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  const auto src = bits::span<const u8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  // TODO: emit a pure read to TB.
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (!(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal) && _tb)
    ;
  bits::memcpy(dest, src);
  return {};
}

Target::Result Dense::write(Address address, bits::span<const u8> src, Operation op) {
  using E = Error;
  auto span = *_config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();
  auto dest = bits::span<u8>{_data.data(), std::size_t(_data.size())}.subspan(offset);
  // Record changes, even if the come from UI. Otherwise, step back fails.
  // Ignore reads from UI, since this device only issues pure reads.
  // Ignore reads from buffer internal operations.
  if (op.type != Operation::Type::BufferInternal && _tb)
    ;
  bits::memcpy(dest, src);
  return {};
}

void Dense::clear(u8 fill) {
  // TODO: emit a "clear" trace to TB.
  _config.fill = fill;
  std::fill(_data.begin(), _data.end(), fill);
}

void Dense::dump(bits::span<u8> dest) const {
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  bits::memcpy(dest, bits::span<const u8>{_data.data(), std::size_t(_data.size())});
}
