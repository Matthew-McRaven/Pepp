#include "core/sim/memory/io/fifo.hpp"
#include <nlohmann/json.hpp>
#include "core/math/bitmanip/enums.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

FIFORegister::FIFO::Iterator::Iterator() : _q(nullptr), _index(-1) {}

FIFORegister::FIFO::Iterator::Iterator(FIFORegister::FIFO *q, size_t index) : _q(q), _index(index) {}

FIFORegister::FIFO::Iterator &FIFORegister::FIFO::Iterator::operator++() {
  _index++;
  return *this;
}

FIFORegister::FIFO::Iterator FIFORegister::FIFO::Iterator::operator++(int) {
  Iterator tmp = *this;
  ++(*this);
  return tmp;
}

bool FIFORegister::FIFO::Iterator::operator!=(const Iterator &other) const { return !(*this == other); }

bool FIFORegister::FIFO::Iterator::operator==(const Iterator &other) const {
  if (_q != other._q) return false;
  else return _index == other._index;
}

bool FIFORegister::FIFO::Iterator::at_end() const { return _index >= _q->size(); }

u8 FIFORegister::FIFO::Iterator::operator*() const { return _q->at(_index); }

u8 FIFORegister::FIFO::Iterator::value_or(u8 def) const {
  if (at_end()) return def;
  else return _q->at(_index);
}

FIFORegister::FIFO::Iterator FIFORegister::FIFO::begin() { return Iterator(this, 0); }

FIFORegister::FIFO::Iterator FIFORegister::FIFO::end() { return Iterator(this, size()); }

void FIFORegister::FIFO::push(u8 value) { _data.write(_max_index++, {&value, 1}); }

u8 FIFORegister::FIFO::at(Address index) const {
  u8 res;
  _data.read(index, bits::span<u8>{&res, 1});
  return res;
}

void FIFORegister::FIFO::clear() { _data.clear(0); }

size_t FIFORegister::FIFO::size() const noexcept {
  return _max_index; // Since we always push to the end, the size is equal to the max index.
}

bool FIFORegister::FIFO::empty() const noexcept {
  return size() == 0; // If the size is 0, then the queue is empty.
}

u8 FIFORegister::FIFO::latest_or(u8 def) const noexcept {
  if (empty()) return def;
  else return at(_max_index - 1);
}

namespace {
Device *create_mmreg(const nlohmann::json &self, System *sys, Device *par) {
  using namespace bits;
  FIFORegister::Configuration cfg;
  try {
    parse_standard_fields(self, cfg);
    if (cfg.basename.empty()) throw ParsingError("FIFORegister must have a basename");
    if (!self.contains("offset") || self["offset"].is_null()) throw ParsingError("FIFORegister must have a offset");
    auto offset = as_u32(self["offset"]);

    cfg.span = AddressSpan{offset, offset};
    if (self.contains("fill") && !self["fill"].is_null()) cfg.fill = as_i8(self["fill"]);
    if (self.contains("direction") && !self["direction"].is_null()) {
      auto dir_str = to_lower(self["direction"].get<std::string>());
      if (dir_str == "none") cfg.direction = FIFORegister::Direction::None;
      else if (dir_str == "in") cfg.direction = FIFORegister::Direction::Input;
      else if (dir_str == "out") cfg.direction = FIFORegister::Direction::Output;
      else if (dir_str == "inout") cfg.direction = FIFORegister::Direction::Input | FIFORegister::Direction::Output;
      else throw ParsingError("FIFORegister direction must be one of: none, in, out, inout");
    }
    if (self.contains("fail_policy") && !self["fail_policy"].is_null()) {
      auto policy_str = to_lower(self["fail_policy"].get<std::string>());
      if (policy_str == "raise_error") cfg.fail_policy = FailPolicy::RaiseError;
      else if (policy_str == "yield_default") cfg.fail_policy = FailPolicy::YieldDefaultValue;
      else throw ParsingError("FIFORegister fail_policy must be one of: raise_error, yield_default");
    }
  } catch (const nlohmann::json::type_error &e) {
    throw ParsingError("Failed to parse dense FIFORegister: " + std::string(e.what()));
  }
  return sys->make_device<FIFORegister>(par, cfg);
}
void prefill_mmreg(nlohmann::json &obj) {
  obj["compatible"] = FIFORegister::compatible;
  obj["basename"];
  obj["offset"];
  obj["fill"] = 0;
  obj["direction"] = "none";
  obj["fail_policy"] = "raise_error";
}
void serialize_mmreg(nlohmann::json &obj, const System *sys, const Device *self) {
  using namespace bits;
  auto casted = dynamic_cast<const FIFORegister *>(self);
  if (!casted) throw std::logic_error("serialize_mmreg called on non-FIFORegister device");
  obj["compatible"] = FIFORegister::compatible;
  obj["basename"] = casted->config().basename;
  obj["offset"] = casted->casted_config().span.lower();
  if (casted->casted_config().fill != 0) obj["fill"] = casted->casted_config().fill;
  switch (casted->casted_config().direction) {
  case FIFORegister::Direction::None: obj["direction"] = "none"; break;
  case FIFORegister::Direction::Input: obj["direction"] = "in"; break;
  case FIFORegister::Direction::Output: obj["direction"] = "out"; break;
  case (FIFORegister::Direction::Input | FIFORegister::Direction::Output): obj["direction"] = "inout"; break;
  default: throw std::logic_error("Invalid FIFORegister direction");
  }
  switch (casted->casted_config().fail_policy) {
  case FailPolicy::YieldDefaultValue: obj["fail_policy"] = "yield_default"; break;
  case FailPolicy::RaiseError: obj["fail_policy"] = "raise_error"; break;
  default: throw std::logic_error("Invalid FIFORegister fail_policy");
  }
}
} // namespace

FIFORegister::FIFORegister(Configuration config) : Device(), _config(config), _input(), _output() {
  _input_it = _input.begin();
  if (_config.span.lower() != _config.span.upper())
    throw std::logic_error("Memory-Mapped Reg must only span single byte.");
}

FIFORegister::FIFO &FIFORegister::input() { return _input; }

FIFORegister::FIFO &FIFORegister::output() { return _output; }

const Device::Configuration &FIFORegister::config() const { return _config; }

const FIFORegister::Configuration &FIFORegister::casted_config() const { return _config; }

const Device::ID FIFORegister::id() const { return _config.id; }

Device::Type FIFORegister::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

u64 FIFORegister::features() const { return 0; }

std::unique_ptr<DeviceSerializer> FIFORegister::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> FIFORegister::make_serializer() {
  DeviceSerializer s{.parser = create_mmreg,
                     .prefill = prefill_mmreg,
                     .serialize = serialize_mmreg,
                     .compatible = FIFORegister::compatible};
  return std::make_unique<DeviceSerializer>(std::move(s));
}

void FIFORegister::set_buffer(Buffer *tb) { _tb = tb; }

const Buffer *FIFORegister::buffer() const { return _tb; }

bool FIFORegister::can_generate_traces() const { return true; }

void FIFORegister::trace(bool enabled) {
  if (_tb) _tb->trace(id(), enabled);
}

bool FIFORegister::traced() const { return _tb ? _tb->traced(id()) : false; }

AddressSpan FIFORegister::span() const { return _config.span; }

void FIFORegister::clear(u8 fill) {
  _input.clear(), _output.clear();
  _input_it = _input.begin();
}

void FIFORegister::dump(bits::span<u8> dest) const {
  using namespace bits;
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  u8 v = _config.fill;
  if (any(_config.direction & FIFORegister::Direction::Output)) {
    _output.latest_or(_config.fill);
  } else if (any(_config.direction & FIFORegister::Direction::Input)) {
    v = _input_it.value_or(_config.fill);
  }
  bits::memcpy(dest, bits::span<const u8>{&v, sizeof(v)});
}

Target::Result FIFORegister::read(Address address, bits::span<u8> dest, Operation op) const {
  using namespace bits;
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);

  u8 src = _config.fill;
  // Only consumes input when the operation is not part of application-internal updates.
  const bool advances_input = !(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal);
  // This is an input register, so we read from the input queue. If the input queue is empty, we might raise an error
  if (any(_config.direction & FIFORegister::Direction::Input)) {
    // If at the end of the input queue, make a decision based on our fail policy.
    if (advances_input && _input_it.at_end()) {
      if (_config.fail_policy == FailPolicy::RaiseError) throw Error(Error::Type::NeedsMMI, address);
      else src = _config.fill;
    } else src = _input_it.value_or(_config.fill);

    if (advances_input) {
      // TODO: emit an impure read to TB.
      if (_tb)
        ;
      if (!(_input_it.at_end())) ++_input_it;
    }
  } else if (any(_config.direction & FIFORegister::Direction::Output)) {
    // If an output register, return the most recently written value.
    src = _output.latest_or(_config.fill);
  }
  bits::memcpy(dest, bits::span<const u8>{&src, 1});
  return {};
}

Target::Result FIFORegister::write(Address address, bits::span<const u8> src, Operation op) {
  using namespace bits;
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);

  // Only advances output when the write isn't related to some app/debugger state change.
  const bool advances_output = !(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal);
  // This is an output reg. Only enqueue value if the operation is not part of an app-internal update.
  if (advances_output && any(_config.direction & FIFORegister::Direction::Output)) {
    // TODO: emit write to TB.
    if (_tb)
      ;
    _output.push(src.front());
  }
  // Ignore writes to non-output registers.
  return {};
}