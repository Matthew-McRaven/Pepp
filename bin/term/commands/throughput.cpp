/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "throughput.hpp"
#include <chrono>
#include <iostream>
#include "core/integers.h"
#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/memory/bus/simplebus.hpp"
#include "core/sim/memory/ram/dense.hpp"
#include "core/sim/memory/ram/sparse.hpp"
#include "core/sim/system.hpp"
#include "sim3/cores/pep/traced_pep10_isa3.hpp"
#include "sim3/subsystems/ram/dense.hpp"

const auto desc_mem = sim::api2::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

const auto desc_cpu = sim::api2::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

const auto span = sim::api2::memory::AddressSpan<quint16>(0, 0xFFFF);

auto make_sim3() {
  int i = 3;
  sim::api2::device::IDGenerator gen = [&i]() { return i++; };
  auto storage = QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
  auto cpu = QSharedPointer<targets::pep10::isa::CPU>::create(desc_cpu, gen);
  cpu->setTarget(storage.data(), nullptr);
  return std::pair{storage, cpu};
};

auto make_core(bool use_sparse) {
  static constexpr auto isa = PepISA3CPU::ISA::Pep10;
  using namespace bits;

  System::Configuration root_cfg{{.basename = "/", .compatible = System::compatible}};
  auto system = std::make_unique<System>(root_cfg);

  PepISA3CPU::Configuration cpu_cfg{Device::Configuration{
                                        .basename = "cpu",
                                        .compatible = PepISA3CPU::compatible,
                                    },
                                    isa, "/memory"};
  auto *cpu = system->make_device<PepISA3CPU>(cpu_cfg, system.get());

  Target *mem = nullptr;
  if (!use_sparse) {
    Dense::Configuration mem_cfg{
        Device::Configuration{
            .basename = "memory",
            .compatible = Dense::compatible,
        },
        0x00,
        AddressSpan(0x0000, 0xffff),
    };
    mem = system->make_device<Dense>(mem_cfg);
  } else {
    Sparse::Configuration mem_cfg{
        Device::Configuration{
            .basename = "memory",
            .compatible = Sparse::compatible,
        },
        0x00,
        AddressSpan(0x0000, 0xffff),
    };
    mem = system->make_device<Sparse>(mem_cfg);
  }

  system->initialize();
  return std::make_tuple(std::move(system), mem, cpu);
}

ThroughputTask::ThroughputTask(WhichVersion ver, QObject *parent) : Task(parent), _version(ver) {}

void ThroughputTask::run() {
  using namespace Qt::StringLiterals;

  // Infinite looping branch to 0.
  static constexpr std::array<u8, 3> SelfBranch{static_cast<u8>(isa::Pep10::Mnemonic::BR), 0x00, 0x00};
  // Program that calculates fib(n/3) in A, truncated to 16 bits of
  // clang-format off
  static constexpr std::array<u8, 12> RMW{
      // Loop preamble
      static_cast<u8>(isa::Pep10::Mnemonic::LDWA),     0x00, 0x01, // Pre-populate A with 1
      // Loop body, which uses stores temporary data in LDWA's operand.
      static_cast<u8>(isa::Pep10::Mnemonic::ADDA) + 1, 0x00, 0x01, // Add A to previous iteration's copy
      static_cast<u8>(isa::Pep10::Mnemonic::STWA) + 1, 0x00, 0x01, // Store copy of A to 0x0001
      static_cast<u8>(isa::Pep10::Mnemonic::BR),       0x00, 0x03  // Loop back to the start
  };
  // clang-format on
  std::span<const u8> program = SelfBranch;
  std::string program_name = "??";
  switch (this->program) {
  case TestProgram::SelfBranch:
    program = SelfBranch;
    program_name = "self-branch";
    break;
  case TestProgram::RMW:
    program = RMW;
    program_name = "read-modify-write loop";
    break;
  }
  fmt::println("Selected program: {}", program_name);

  std::chrono::high_resolution_clock::time_point start;
  switch (_version) {
  case WhichVersion::Sim3: start = do_sim3(program); break;
  case WhichVersion::Core: start = do_core(program); break;
  }
  const auto end = std::chrono::high_resolution_clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  const auto dt = 1.0 / (ms.count() / 1000.0);
  const auto locale = std::locale("en_US.UTF-8");
  fmt::println("Duration: {} ms", ms.count());
  std::cout << fmt::format(locale, "Instructions: {:L}\n", maxInstr);
  std::cout << fmt::format(locale, "Throughput: {:L} instructions/second\n", (i32)(dt * maxInstr));

  emit finished(0);
}

std::chrono::high_resolution_clock::time_point ThroughputTask::do_sim3(std::span<const u8> program) {
  static constexpr sim::api2::memory::Operation rw = {
      .type = sim::api2::memory::Operation::Type::Standard,
      .kind = sim::api2::memory::Operation::Kind::data,
  };
  fmt::println("Simulator: sim3");
  auto env = nullptr;
  // Add some spurious breakpoints which will not be hit
  // auto debugger = std::make_shared<pepp::debug::Debugger>(env);
  /*pepp::debug::Parser p(*debugger->cache);
  for (int it = 0; it < 128; it++) debugger->bps->addBP(2048 + it);
  auto bp = p.compile("10 + 2");
  if (bp == nullptr) {
    std::cerr << "Failed to compile breakpoint expression.\n";
    emit finished(1);
    return;
  }*/
  // debugger->bps->addBP(0 /*, bp.get()*/);
  auto [mem, cpu] = make_sim3();
  // cpu->setDebugger(&*debugger);
  cpu->regs()->clear(0);
  cpu->csrs()->clear(0);
  mem->write(0, program, rw);
  const auto start = std::chrono::high_resolution_clock::now();
  for (int it = 0; it < maxInstr; it++) cpu->clock(it);
  return start;
}

std::chrono::high_resolution_clock::time_point ThroughputTask::do_core(std::span<const u8> program) {
  static constexpr auto rw = Operation{Operation::Type::Standard, Operation::Kind::data};
  fmt::println("Simulator: core");
  auto [system, mem, cpu] = make_core(this->use_sparse);
  cpu->write_register(isa::Pep10::Register::PC, 0x0000);
  mem->write(0x0000, program, rw);
  // We are untraced, provide explicit hints to avoid recording.
  dynamic_cast<Traceable *>(mem)->on_traced_changed(false);
  cpu->on_traced_changed(false);
  cpu->csrs()->on_traced_changed(false);
  cpu->registers()->on_traced_changed(false);
  cpu->has_bps = has_bps;
  const auto start = std::chrono::high_resolution_clock::now();
  for (int it = 0; it < maxInstr; it++) cpu->clock_tick(PulseSchedule::PulseIndex{(u64)it}, it);
  fmt::println("Filter hits: {}", cpu->filter_hits());
  return start;
}
