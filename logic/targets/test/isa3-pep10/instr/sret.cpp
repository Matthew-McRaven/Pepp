/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
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

#include <QTest>
#include <QtCore>

#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
auto desc_mem = sim::api::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

auto desc_cpu = sim::api::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

auto span = sim::api::memory::Target<quint16>::AddressSpan{
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

sim::api::memory::Operation rw = {.speculative = false,
                                  .kind =
                                      sim::api::memory::Operation::Kind::data,
                                  .effectful = false};

class ISA3Pep10_SRET : public QObject {
  Q_OBJECT
private slots:
  void i() {
    auto [mem, cpu] = make();

    // Can't capture CPU directly b/c structured bindings.
    auto _cpu = cpu;
    auto rreg = [&](isa::Pep10::Register reg) -> quint16 {
      quint16 tmp = 0;
      targets::pep10::isa::readRegister(_cpu->regs(), reg, tmp, rw);
      return tmp;
    };

    quint8 tmp8 = 0;
    quint16 tmp = 0;
    auto tmpSpan = bits::span<quint8>{reinterpret_cast<quint8*>(tmp), sizeof(tmp)};
    const quint8 truth[] = {/*NZVC*/ 0b1101, /*A*/ 0x11,0x22, /*X*/ 0xBA, 0xAD, /*PC*/ 0xCA,0xDE, /*sp*/ 0xFE, 0xED,(quint8) isa::Pep10::Mnemonic::SCALL};
    quint8 buf[sizeof(truth)];
    auto program = std::array<quint8, 1>{(quint8) isa::Pep10::Mnemonic::SRET};

    cpu->regs()->clear(0);
    cpu->csrs()->clear(0);
    QVERIFY(targets::pep10::isa::writeRegister(cpu->regs(), isa::Pep10::Register::SP, 0x8086-10, rw).completed);
    QVERIFY(mem->write(0x0000, {program.data(), program.size()}, rw).completed);
    QVERIFY(mem->write(0x8086-10, {truth, sizeof(truth)}, rw).completed);

    auto tick = cpu->tick(0);
    QCOMPARE(tick.error, sim::api::tick::Error::Success);

    QVERIFY(targets::pep10::isa::readPackedCSR(cpu->csrs(),tmp8,rw).completed);
    QCOMPARE(tmp8, truth[0]);
    QVERIFY(targets::pep10::isa::readRegister(cpu->regs(), isa::Pep10::Register::A, tmp, rw).completed);
    QCOMPARE(tmp, truth[1]<<8|truth[2]);
    QVERIFY(targets::pep10::isa::readRegister(cpu->regs(), isa::Pep10::Register::X, tmp, rw).completed);
    QCOMPARE(tmp, truth[3]<<8|truth[4]);
    QVERIFY(targets::pep10::isa::readRegister(cpu->regs(), isa::Pep10::Register::PC, tmp, rw).completed);
    QCOMPARE(tmp, truth[5]<<8|truth[6]);
    QVERIFY(targets::pep10::isa::readRegister(cpu->regs(), isa::Pep10::Register::SP, tmp, rw).completed);
    QCOMPARE(tmp, truth[7]<<8|truth[8]);

    QVERIFY(mem->read((quint16)isa::Pep10::MemoryVectors::SystemStackPtr, {reinterpret_cast<quint8*>(&tmp), sizeof(tmp)}, rw).completed);

    tmp = bits::hostOrder() != bits::Order::BigEndian ? bits::byteswap(tmp)
                                                      : tmp;
    QCOMPARE(tmp, 0x8086);
  }
};

#include "sret.moc"

QTEST_MAIN(ISA3Pep10_SRET)
