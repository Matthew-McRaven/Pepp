#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

#include "components/delta/base.hpp"
#include "helper.hpp"
#include "outcome_helper.hpp"

namespace components::storage {

template <typename offset_t, bool enable_history = true, typename val_size_t = uint8_t>
requires(components::storage::UnsignedIntegral<offset_t> &&components::storage::Integral<val_size_t>) class Base {
  public:
    // TODO: Rule of 5.
    // TODO: Copy-swap.
    // TODO: Ban copying "C.67: A polymorphic class should suppress copying"
    Base(offset_t max_offset);
    virtual ~Base() noexcept = default;
    virtual void clear(val_size_t fill_val = 0) = 0;
    // Set / Get functions cannot trigger delta generation.
    // If you want something to be recorded as a delta, you must use read/write.
    virtual result<val_size_t> get(offset_t offset) const = 0;
    virtual result<void> set(offset_t offset, val_size_t value) = 0;
    // Read / Write functions that may generate signals or trap for IO.
    // They also trigger delta generation.
    virtual result<val_size_t> read(offset_t offset) const = 0;
    virtual result<void> write(offset_t offset, val_size_t value) = 0;

    // Return the most deeply nested device of a grouped device. If the device is not a grouped device, return this.
    // If offset is out of bounds, return an OOB-related error code.
    virtual result<const Base *> device_at(offset_t offset) const;
    virtual result<Base *> device_at(offset_t offset);

    // Provide  building block of `undo` using layered deltas.
    virtual bool deltas_enabled() const = 0;
    virtual result<void> clear_delta() = 0;
    virtual result<std::unique_ptr<components::delta::Base<offset_t, val_size_t>>> take_delta() = 0;

    // Number of bytes contained by this chip
    virtual offset_t max_offset() const;
    // Change the size of the chip at runtime, to avoid creating and deleting
    // an excessive number of chip instances.
    // Resizing the underlying storage may indeed throw due to lack of memory or a myriad of other STL problems.
    // If this is the case, there is absolutely nothing we can do to fix application state -- we are out of memory.
    // Just terminate directly.
    virtual result<void> resize(offset_t new_offset) = 0;

  protected:
    offset_t _max_offset;
};

// Will "wrap-around" if bytes exceed maximum offset.
template <typename offset_t, bool enable_history, typename val_size_t = uint8_t>
requires(UnsignedIntegral<offset_t> &&Integral<val_size_t>) result<void> load_bytes(
    components::storage::Base<offset_t, enable_history, val_size_t> &storage, const std::vector<uint8_t> &bytes,
    uint16_t offset);

}; // namespace components::storage

#include "base.tpp"