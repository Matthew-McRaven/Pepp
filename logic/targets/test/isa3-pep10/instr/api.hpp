#pragma once
#include "bits/operations/swap.hpp"
#include "sim/device/dense.hpp"
#include "targets/pep10/isa3/cpu.hpp"
#include "targets/pep10/isa3/helpers.hpp"
#include <qtypes.h>

auto desc_mem = sim::api2::device::Descriptor{
    .id = 1,
    .baseName = "ram",
    .fullName = "/ram",
};

auto desc_cpu = sim::api2::device::Descriptor{
    .id = 2,
    .baseName = "cpu",
    .fullName = "/cpu",
};

auto span = sim::api2::memory::AddressSpan<quint16>{
    .minOffset = 0,
    .maxOffset = 0xFFFF,
};

sim::api2::memory::Operation rw = {
    .type = sim::api2::memory::Operation::Type::Standard,
    .kind = sim::api2::memory::Operation::Kind::data,
};

inline auto make = []() {
    int i = 3;
    sim::api2::device::IDGenerator gen = [&i]() { return i++; };
    auto storage = QSharedPointer<sim::memory::Dense<quint16>>::create(desc_mem, span);
    auto cpu = QSharedPointer<targets::pep10::isa::CPU>::create(desc_cpu, gen);
    cpu->setTarget(storage.data(), nullptr);
    return std::pair{storage, cpu};
};

inline auto reg(QSharedPointer<targets::pep10::isa::CPU> cpu, isa::Pep10::Register reg) -> quint16
{
    quint16 tmp = 0;
    targets::pep10::isa::readRegister(cpu->regs(), reg, tmp, rw);
    return tmp;
};

inline quint16 csr(QSharedPointer<targets::pep10::isa::CPU> cpu, isa::Pep10::CSR csr)
{
    bool tmp = 0;
    targets::pep10::isa::readCSR(cpu->csrs(), csr, tmp, rw);
    return tmp;
};
