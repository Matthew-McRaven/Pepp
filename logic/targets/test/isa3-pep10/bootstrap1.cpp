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
sim::api::memory::Operation rw = {.speculative = false,
    .kind =
    sim::api::memory::Operation::Kind::data,
    .effectful = false};

class Targets_ISA3Pep10_Bootstrap1 : public QObject {
Q_OBJECT
private slots:
  void smoke() {
    sim::api::device::ID id = 0;
    auto nextID = [&id]() { return id++; };
    auto desc_mem = sim::api::device::Descriptor{.id = nextID(),
        .compatible = nullptr,
        .baseName = "dev",
        .fullName = "/dev"};
    auto span = sim::api::memory::Target<quint16>::AddressSpan{
        .minOffset = 0, .maxOffset = 0xFFFF};
    sim::memory::Dense<quint16> mem(desc_mem, span, (int) isa::Pep10::Mnemonic::NOP);
    auto desc_cpu = sim::api::device::Descriptor{.id = nextID(),
        .compatible = nullptr,
        .baseName = "cpu",
        .fullName = "/cpu"};
    targets::pep10::isa::CPU cpu(desc_cpu, nextID);
    cpu.setTarget(&mem);
    auto regs = cpu.regs();
    quint16 tmp = 0;

    using Register = isa::Pep10::Register;
    // Check that PC is incremented when executing NOP.
    QVERIFY(targets::pep10::isa::readRegister(regs, Register::PC, tmp, rw)
                .completed);
    QCOMPARE(tmp, 0);

    auto tick = cpu.tick(0);

    QVERIFY(targets::pep10::isa::readRegister(regs, Register::PC, tmp, rw)
                .completed);
    QCOMPARE(tmp, 1);

    // Check that A can be modified.
    quint8 v = (quint8) isa::Pep10::Mnemonic::NOTA;
    QVERIFY(mem.write(0x01, {&v, 1}, rw).completed);
    QVERIFY(targets::pep10::isa::readRegister(regs, Register::A, tmp, rw)
                .completed);
    QCOMPARE(tmp, 0);

    tick = cpu.tick(1);

    QVERIFY(targets::pep10::isa::readRegister(regs, Register::A, tmp, rw)
                .completed);
    QCOMPARE(tmp, 0xFFFF);
  }
};

#include "bootstrap1.moc"

QTEST_MAIN(Targets_ISA3Pep10_Bootstrap1)
