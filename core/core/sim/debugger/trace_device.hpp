#pragma once
#include <memory>
#include "core/sim/api/device.hpp"
#include "core/sim/debugger/tvm_tracebuffer.hpp"

namespace trace {

// The synthetic device that owns a tvm::TraceBuffer and exposes it as a Device type.
// Like other sythetic devices, the owning System will create one automatically without needing any configuration JSON
// for the device.
//
// This class exposes only a subset of the TraceBuffer's functionality, meant to allows us (the GUI) to iterate changes
// and release them once the UI is caught up. It also provides a top-level API to trace individual devices. It
// intentionally does not have any way to create data. If you need to create a trace, use a trace recorder.
class BufferDevice final : public Device {
public:
  static constexpr Device::Type TypeMask = Device::Type::TraceBuffer;
  static const inline std::string compatible = "debug,trace-buffer";
  struct Configuration : public Device::Configuration {
    // Upper bound on retained history: the ring holds this many slots, each up to ~16k programs.
    size_t ring_size = 4;
  };

  explicit BufferDevice(Configuration cfg);
  ~BufferDevice() override = default;

  tvm::TraceBuffer &buffer();
  const tvm::TraceBuffer &buffer() const;

  // --- Iteration ---
  tvm::Cursor cursor() const { return buffer().cursor(); }
  tvm::TraceBuffer::CursorRange range(tvm::Cursor from, tvm::Cursor to) const { return buffer().range(from, to); }
  // Release every slot before up_to.slot. Any Buffer::Location handed out for those slots dies here.
  void acknowledge(tvm::Cursor up_to) { buffer().acknowledge(up_to); }

  // --- Which devices are recorded ---
  // Asked of the device whose bytes change, not of the initiator: if the memory being written is traced, the
  // transaction is worth keeping, and the initiator only decides which recording it lands in.
  void trace(Device::ID device, bool enabled = true) { buffer().trace(device, enabled); }
  bool traced(Device::ID device) const { return buffer().traced(device); }

  // --- Inspection ---
  std::size_t instruction_count() const { return buffer().instruction_count(); }
  float ring_occupancy() const { return buffer().ring_occupancy(); }

  // Device interface. The buffer is built in initialize() rather than the constructor because it needs the System's
  // BufferManager, which is only reachable once the tree exists.
  void initialize(System *sys) override;
  const Device::Configuration &config() const override { return _config; }
  const Device::ID id() const override { return _config.id; }
  Device::Type type() const override { return Device::Type::TraceBuffer; }
  std::unique_ptr<DeviceSerializer> serializer() const override;

private:
  Configuration _config;
  // Null until initialize() runs.
  std::unique_ptr<tvm::TraceBuffer> _tb;
};

} // namespace trace
