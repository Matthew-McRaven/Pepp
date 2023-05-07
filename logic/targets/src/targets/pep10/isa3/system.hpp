#pragma once
#include "sim/api.hpp"
#include <elfio/elfio.hpp>

namespace obj {
class MemoryRegion;
class AddressedIO;
} // namespace obj
namespace sim {
namespace memory {
template <typename Address> class Dense;
template <typename Address> class SimpleBus;
template <typename Address> class Input;
template <typename Address> class Output;
template <typename Address> class ReadOnly;
} // namespace memory
} // namespace sim
namespace targets::pep10::isa {
class CPU;
class System : public sim::api::System<quint16> {
public:
  System(QList<obj::MemoryRegion> regions, QList<obj::AddressedIO> mmios);
  // System interface
  std::pair<sim::api::tick::Type, sim::api::tick::Result>
  tick(sim::api::Scheduler::Mode mode) override;
  sim::api::tick::Type currentTick() const override;
  sim::api::device::ID nextID() override;
  sim::api::device::IDGenerator nextIDGenerator() override;
  void setTraceBuffer(sim::api::trace::Buffer *buffer) override;

  // Set default register values, modify dispatcher / loader behavior.
  void setBootFlagAddress(quint16 addr);
  void setBootFlags(bool enableLoader = true, bool enableDispatcher = true);
  std::optional<quint16> getBootFlagAddress();
  quint16 getBootFlags() const;
  void init();
  CPU *cpu();
  sim::memory::SimpleBus<quint16> *bus();
  QStringList inputs() const;
  sim::memory::Input<quint16> *input(QString name);
  QStringList outputs() const;
  sim::memory::Output<quint16> *output(QString name);

private:
  sim::api::device::ID _nextID = 0;
  sim::api::device::IDGenerator _nextIDGenerator = [this]() {
    return _nextID++;
  };
  sim::api::tick::Type _tick = 0;
  std::optional<quint16> _bootFlg = std::nullopt;

  QSharedPointer<CPU> _cpu = nullptr;
  QSharedPointer<sim::memory::SimpleBus<quint16>> _bus = nullptr;
  QVector<QSharedPointer<sim::memory::Dense<quint16>>> _rawMemory = {};
  QVector<QSharedPointer<sim::memory::ReadOnly<quint16>>> _readonly = {};

  QMap<QString, QSharedPointer<sim::memory::Input<quint16>>> _mmi = {};
  QMap<QString, QSharedPointer<sim::memory::Output<quint16>>> _mmo = {};
};

// loadUserImmediate bypasses loading user program to DDR.
QSharedPointer<System> systemFromElf(ELFIO::elfio &elf, bool loadUserImmediate);
} // namespace targets::pep10::isa
