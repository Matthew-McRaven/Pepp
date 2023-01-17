#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>
#include <stdint.h>

#include <boost/range/adaptor/indexed.hpp>

#include "./helper.hpp"
#include "cabi/target.h"
#include "cabi/trace.h"

namespace device::storage {

template<typename offset_t, typename val_size_t = uint8_t> requires (device::storage::UnsignedIntegral<offset_t>
    && device::storage::Integral<val_size_t>) class Base {
public:
  // TODO: Rule of 5.
  // TODO: Copy-swap.
  // TODO: Ban copying "C.67: A polymorphic class should suppress copying"
  Base(offset_t max_offset);
  virtual ~Base() noexcept = default;
  virtual void clear(val_size_t fill_val = 0) = 0;
  virtual CTarget as_target() = 0;
  virtual device_id_t device_id() const = 0;
  virtual CDevice as_device() = 0;

  // Accessors
  virtual access_result read(offset_t offset, void *data_ptr, uint8_t length, operation op) = 0;
  virtual access_result write(offset_t offset, const void *data_ptr, uint8_t length, operation op) = 0;
  // Return the most deeply nested device of a grouped device. If the device is not a grouped device, return this.
  // If offset is out of bounds, return an OOB-related error code.
  virtual const Base *device_at(offset_t offset) const;
  virtual Base *device_at(offset_t offset);

  // Provide  building block of `undo` using layered deltas.
  virtual void set_trace_buffer(CTraceBuffer) = 0;
  virtual void set_interposer(CInterposer) = 0;

  // First addressable value in this device
  virtual offset_t base() const;
  // Number of bytes contained by this chip
  virtual offset_t max_offset() const;

  // Change the size of the storage target at runtime, to avoid creating and deleting
  // an excessive number of target  instances.
  // Resizing the underlying storage may indeed throw due to lack of memory or a myriad of other STL problems.
  // If this is the case, we return false, giving the application a choice of what to do in these memory-constrained
  // cases. Likely, the best thing to do is crash with an error message.
  virtual bool resize(offset_t new_offset) = 0;

protected:
  offset_t _max_offset;
};

template<typename offset_t, typename val_size_t>
requires (device::storage::UnsignedIntegral<offset_t> && device::storage::Integral<val_size_t>)
device::storage::Base<offset_t, val_size_t>::Base(offset_t max_offset)
    :  _max_offset(max_offset) {}

template<typename offset_t, typename val_size_t>
requires (device::storage::UnsignedIntegral<offset_t> && device::storage::Integral<val_size_t>)
Base<offset_t, val_size_t> *device::storage::Base<offset_t, val_size_t>::device_at(offset_t offset) {
  // TODO: Change to OOBAccess
  if (offset > max_offset())
    return nullptr;
  else
    return this;
}

template<typename offset_t, typename val_size_t>
requires (device::storage::UnsignedIntegral<offset_t> && device::storage::Integral<val_size_t>)
const Base<offset_t, val_size_t> *device::storage::Base<offset_t, val_size_t>::device_at(offset_t offset) const {
  // TODO: Change to OOBAccess
  if (offset > max_offset())
    return nullptr;
  else
    return this;
}

template<typename offset_t, typename val_size_t>
requires (device::storage::UnsignedIntegral<offset_t> && device::storage::Integral<val_size_t>)
offset_t device::storage::Base<offset_t, val_size_t>::base() const { return 0; }

template<typename offset_t, typename val_size_t>
requires (device::storage::UnsignedIntegral<offset_t> && device::storage::Integral<val_size_t>)
offset_t device::storage::Base<offset_t, val_size_t>::max_offset() const { return _max_offset; }

// Will "wrap-around" if bytes exceed maximum offset.
template<typename offset_t>
struct load_bytes_result {
  offset_t last_byte_index; // Index of the last byte that was loaded (may not be bytes.size())
  access_result result; // When last_byte_index is not at the end, why did loading stop part way through?
};
template<typename offset_t, typename val_size_t>
requires (device::storage::UnsignedIntegral<offset_t> && device::storage::Integral<val_size_t>)
load_bytes_result<offset_t> load_bytes(device::storage::Base<offset_t, val_size_t> &storage,
                                       const std::vector<uint8_t> &bytes,
                                       uint16_t offset) {
  // Must add 1 to modulus, otherwise we won't be able to "hit" the maximum address.
  assert(storage.max_offset() != 0xFFFF'FFFF'FFFF'FFFF);
  const uint64_t modulo = storage.max_offset() + 1;
  for (auto [index, byte] : bytes | boost::adaptors::indexed(0)) {
    auto res = storage.set((offset + index) % modulo, byte);
    // TODO: Implement when we have a storage API
    if (res.has_failure())
      return res.error().clone();
  }
  return {.last_byte_index=bytes.size(), .result={.advance=true, .pause=false, .sync=false, .error=access_error::Success}};
}

}; // namespace device::storage
