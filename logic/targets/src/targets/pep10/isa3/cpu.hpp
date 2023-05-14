#pragma once
#include "isa/pep10.hpp"
#include "sim/api.hpp"
#include "sim/device/dense.hpp"
namespace targets::pep10::isa {
class CPU : public sim::api::tick::Listener,
            sim::api::trace::Producer,
            sim::api::memory::Initiator<quint16> {

public:
  CPU(sim::api::device::Descriptor device, sim::api::device::IDGenerator gen);
  ~CPU() = default;
  CPU(CPU &&other) noexcept = default;
  CPU &operator=(CPU &&other) = default;
  CPU(const CPU &) = delete;
  CPU &operator=(const CPU &) = delete;

  sim::api::memory::Target<quint8> *regs();
  sim::api::memory::Target<quint8> *csrs();

  // Listener interface
  const sim::api::tick::Source *getSource() override;
  void setSource(sim::api::tick::Source *) override;
  sim::api::tick::Result tick(sim::api::tick::Type currentTick) override;

  // Producer interface
  void setTraceBuffer(sim::api::trace::Buffer *tb) override;
  void trace(bool enabled) override;
  quint8 packetSize(sim::api::packet::Flags flags) const override;
  bool applyTrace(bits::span<const quint8> payload,
                  sim::api::packet::Flags flags) override;
  bool unapplyTrace(bits::span<const quint8> payload,
                    sim::api::packet::Flags flags) override;

  // Initiator interface
  void setTarget(sim::api::memory::Target<quint16> *target) override;
  void setTarget(void *port,
                 sim::api::memory::Target<quint16> *target) override;

private:
  sim::api::device::Descriptor _device;
  sim::memory::Dense<quint8> _regs, _csrs;
  sim::api::memory::Target<quint16> *_memory;

  sim::api::tick::Source *_clock = nullptr;
  sim::api::trace::Buffer *_tb = nullptr;

  quint16 readReg(::isa::Pep10::Register reg);
  void writeReg(::isa::Pep10::Register reg, quint16 val);
  bool readCSR(::isa::Pep10::CSR csr);
  void writeCSR(::isa::Pep10::CSR csr, bool val);
  quint8 readPackedCSR();
  void writePackedCSR(quint8 val);

  sim::api::tick::Result unaryDispatch(quint8 is);
  sim::api::tick::Result nonunaryDispatch(quint8 is, quint16 os, quint16 pc);
  sim::api::memory::Result decodeStoreOperand(quint8 is, quint16 os,
                                              quint16 &decoded);
  sim::api::memory::Result decodeLoadOperand(quint8 is, quint16 os,
                                             quint16 &decoded);
};
} // namespace targets::pep10::isa
