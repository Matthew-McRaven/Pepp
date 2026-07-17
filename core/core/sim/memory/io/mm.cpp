#include "mm.hpp"
#include <nlohmann/json.hpp>
#include "core/math/bitmanip/enums.hpp"
#include "core/sim/memory/errors.hpp"
#include "core/sim/memory/ram/sparse.hpp"
#include "core/sim/systemparser.hpp"

IOQueue::Iterator::Iterator() : _q(nullptr), _index(-1) {}

IOQueue::Iterator::Iterator(IOQueue *q, size_t index) : _q(q), _index(index) {}

IOQueue::Iterator &IOQueue::Iterator::operator++() {
  _index++;
  return *this;
}

IOQueue::Iterator IOQueue::Iterator::operator++(int) {
  Iterator tmp = *this;
  ++(*this);
  return tmp;
}

bool IOQueue::Iterator::operator!=(const Iterator &other) const {
  if (_q != other._q) return true;
  else if (at_end() && other.at_end()) return false;
  else return _index != other._index;
}

bool IOQueue::Iterator::at_end() const { return _index >= _q->size(); }

u8 IOQueue::Iterator::operator*() const { return _q->at(_index); }

u8 IOQueue::Iterator::value_or(u8 def) const {
  if (at_end()) return def;
  else return _q->at(_index);
}

IOQueue::Iterator IOQueue::begin() { return Iterator(this, 0); }

IOQueue::Iterator IOQueue::end() {
  return Iterator(this, -1); // end iterator is always at index -1, which is beyond the end of the queue.
}

void IOQueue::push(u8 value) { _data->write(_max_index++, {&value, 1}); }

u8 IOQueue::at(Address index) const {
  u8 res;
  _data->read(index, bits::span<u8>{&res, 1});
  return res;
}

void IOQueue::clear() { _data->clear(0); }

size_t IOQueue::size() const {
  return _max_index; // Since we always push to the end, the size is equal to the max index.
}

bool IOQueue::empty() const {
  return size() == 0; // If the size is 0, then the queue is empty.
}

u8 IOQueue::latest_or(u8 def) const {
  if (empty()) return def;
  else return at(_max_index - 1);
}

MemoryMappedRegister::MemoryMappedRegister(Configuration config) : Device(), _config(config), _input(), _output() {
  _input_it = _input.begin();
  if (_config.span.lower() != _config.span.upper())
    throw std::logic_error("Memory-Mapped Reg must only span single byte.");
}

const Device::Configuration &MemoryMappedRegister::config() const { return _config; }

const Device::ID MemoryMappedRegister::id() const { return _config.id; }

Device::Type MemoryMappedRegister::type() const {
  using namespace bits;
  using T = Device::Type;
  return T::MemoryTarget | T::Traceable;
}

u64 MemoryMappedRegister::features() const { return 0; }

std::unique_ptr<DeviceSerializer> MemoryMappedRegister::serializer() const { return make_serializer(); }

std::unique_ptr<DeviceSerializer> MemoryMappedRegister::make_serializer() { return nullptr; }

void MemoryMappedRegister::set_buffer(Buffer *tb) { _tb = tb; }

const Buffer *MemoryMappedRegister::buffer() const { return _tb; }

bool MemoryMappedRegister::can_generate_traces() const { return true; }

void MemoryMappedRegister::trace(bool enabled) {
  if (_tb) _tb->trace(id(), enabled);
}

bool MemoryMappedRegister::traced() const { return _tb ? _tb->traced(id()) : false; }

AddressSpan MemoryMappedRegister::span() const { return _config.span; }

void MemoryMappedRegister::clear(u8 fill) {
  _input.clear(), _output.clear();
  _input_it = _input.begin();
}

void MemoryMappedRegister::dump(bits::span<u8> dest) const {
  using namespace bits;
  if (dest.size() <= 0) throw std::logic_error("dump requires non-0 size");
  u8 v = _config.fill;
  if (any(_config.direction & MemoryMappedRegister::IODirection::Output)) {
    _output.latest_or(_config.fill);
  } else if (any(_config.direction & MemoryMappedRegister::IODirection::Input)) {
    v = _input_it.value_or(_config.fill);
  }
  bits::memcpy(dest, bits::span<const u8>{&v, sizeof(v)});
}

Target::Result MemoryMappedRegister::read(Address address, bits::span<u8> dest, Operation op) const {
  using namespace bits;
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, dest.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();

  u8 src = _config.fill;
  // Only consumes input when the operation is not part of application-internal updates.
  const bool advances_input = !(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal);
  // This is an input register, so we read from the input queue. If the input queue is empty, we might raise an error
  if (any(_config.direction & MemoryMappedRegister::IODirection::Input)) {
    // value_or will always return a value, even if at the end of the input.
    src = _input_it.value_or(_config.fill);
    // Rather than have this check on both sides of the read (one to catch at_end(), one to increment), perform both
    // checks here. If the empty, we perform a spurious read of the input queue (without incrementing any iterators!)
    if (advances_input) {
      if (_input_it.at_end()) {
        // TODO: throw exception that we ran out of MMI!
      }
      // TODO: emit an impure read to TB.
      if (_tb)
        ;
      ++_input_it;
    }
  } else if (any(_config.direction & MemoryMappedRegister::IODirection::Output)) {
    // If an output register, return the most recently written value.
    src = _output.latest_or(_config.fill);
  }
  bits::memcpy(dest, bits::span<const u8>{&src, 1});
  return {};
}

Target::Result MemoryMappedRegister::write(Address address, bits::span<const u8> src, Operation op) {
  using namespace bits;
  using E = Error;
  const auto span = _config.span;
  // Length is 1-indexed, address are 0, so must offset by -1.
  const auto max_addr = (address + std::max<Address>(0, src.size() - 1));
  if (address < span.lower() || max_addr > span.upper()) throw E(E::Type::OOBAccess, address);
  const auto offset = address - span.lower();

  // Only advances output when the write isn't related to some app/debugger state change.
  const bool advances_output = !(op.type == Operation::Type::Application || op.type == Operation::Type::BufferInternal);
  // This is an input register, so we read from the input queue. If the input queue is empty, we might raise an error
  if (any(_config.direction & MemoryMappedRegister::IODirection::Output)) {
    // TODO: emit write to TB.
    if (_tb)
      ;
    _output.push(src.front());
  }
  // Ignore writes to non-output registers.
  return {};
}