#include "throughput.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include <chrono>
#include <iostream>
const auto desc_mem = sim::api::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

const auto desc_cpu = sim::api::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

const auto span = sim::api::memory::Target<quint16>::AddressSpan{
    .minOffset = 0,
    .maxOffset = 0xFFFF,
};

auto make = []() {
  int i = 3;
  sim::api::device::IDGenerator gen = [&i]() { return i++; };
  auto storage =
      QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
  auto cpu = QSharedPointer<targets::pep10::isa::CPU>::create(desc_cpu, gen);
  cpu->setTarget(storage.data());
  return std::pair{storage, cpu};
};

const sim::api::memory::Operation rw = {
    .speculative = false,
    .kind = sim::api::memory::Operation::Kind::data,
    .effectful = false};

ThroughputTask::ThroughputTask(QObject *parent) : Task(parent) {}

void ThroughputTask::run() {
  auto [mem, cpu] = make();
  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);
  // Infinite looping branch to 0.
  auto program = std::array<quint8, 3>{0b0001'1100, 0x00, 0x00};
  Q_ASSERT(mem->write(0, {program.data(), program.size()}, rw).completed);
  auto start = std::chrono::high_resolution_clock::now();
  auto maxInstr = 1'000'000;
  for (int it = 0; it < maxInstr; it++) {
    cpu->tick(it);
  }
  auto end = std::chrono::high_resolution_clock::now();
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  auto dt = 1.0 / (ms.count() / 1000.0);
  std::cout << u"Duration was: %1\n"_qs.arg(ms.count()).toStdString();
  std::cout << u"Throughput was: %1\n"_qs.arg(dt * maxInstr).toStdString();

  emit finished(0);
}
