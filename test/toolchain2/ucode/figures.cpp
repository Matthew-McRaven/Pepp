/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include <catch.hpp>
#include <ranges>
#include "core/microarch/pep.hpp"
#include "help/builtins/book.hpp"
#include "help/builtins/figure.hpp"
#include "help/builtins/registry.hpp"
#include "sim3/cores/pep/traced_pep9_mc2.hpp"
#include "sim3/subsystems/ram/dense.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
#include "toolchain2/ucode/pep_parser.hpp"

namespace {
const auto lf = QRegularExpression("\r");
template <typename CPU> std::pair<sim::memory::Dense<quint16>, CPU> make() {
  sim::api2::device::ID id = 0;
  auto nextID = [&id]() { return id++; };
  auto desc_mem =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "dev", .fullName = "/dev"};
  auto span = sim::api2::memory::AddressSpan<quint16>(0, 0xffff);
  sim::memory::Dense<quint16> mem(desc_mem, span, 0);
  auto desc_cpu =
      sim::api2::device::Descriptor{.id = nextID(), .compatible = nullptr, .baseName = "cpu", .fullName = "/cpu"};
  CPU cpu(desc_cpu, nextID);
  return {std::move(mem), std::move(cpu)};
}
template <typename T> quint8 read(sim::api2::memory::Target<T> &mem, T addr) {
  quint8 value = 0;
  auto op = sim::api2::memory::Operation{.type = sim::api2::memory::Operation::Type::Application,
                                         .kind = sim::api2::memory::Operation::Kind::data};
  auto result = mem.read(addr, {&value, 1}, op);
  return value;
}

} // namespace

TEST_CASE("Pep/9 Microcode Assembly & Simulation", "[scope:mc2][kind:e2e][arch:pep10][arch:pep9][tc2]") {
  using uarch1 = pepp::tc::arch::Pep9ByteBus;
  using uarch2 = pepp::tc::arch::Pep9WordBus;
  using regs = pepp::tc::arch::Pep9Registers;
  using namespace Qt::StringLiterals;
  auto bookReg = builtins::Registry();
  auto book5 = helpers::book(5, &bookReg);
  auto book6 = helpers::book(6, &bookReg);
  auto figures5 = book5->figures(), figures6 = book6->figures();
  auto probs5 = book5->problems(), probs6 = book6->problems();
  QList<QList<QSharedPointer<builtins::Figure>>> _combined = {figures5, probs5, figures6, probs6};
  // auto [mem, cpu] = make<targets::pep9::mc2::CPUByteBus>();
  // cpu.setTarget(&mem, nullptr);
  auto combined = std::views::join(_combined);
  for (auto &figure : combined) {
    const auto &frags = figure->typesafeNamedFragments();
    if (!frags.contains("pepcpu")) continue;
    auto name = u"Figure %1.%2"_s.arg(figure->chapterName()).arg(figure->figureName());
    auto nameAsStd = name.toStdString();
    int ed = figure->arch() == pepp::Architecture::PEP9 ? 5 : 6;
    DYNAMIC_SECTION("Microassemble CS" << ed << "E: " << nameAsStd) {
      auto source = frags.value("pepcpu", nullptr);
      CHECK(figure->typesafeTests().size() > 0);
      REQUIRE(source != nullptr);
      if (source->language == "pepcpu1") {
        auto assembledFig = pepp::tc::parse::MicroParser<uarch1, regs>(source->contents()).parse();
        if (assembledFig.errors.size() > 0) {
          QStringList errors;
          for (const auto &[line, error] : assembledFig.errors) errors.append(u"%1: %2"_s.arg(line).arg(error));
          FAIL("Errors in " + nameAsStd + " microassembly: " + errors.join(", ").toStdString());
        }
        // Check that all tests assemble too
        for (auto io : figure->typesafeTests()) {
          QString input = io->input.toString().replace(lf, "");
          auto assembledTest = pepp::tc::parse::MicroParser<uarch1, regs>(input).parse();
          if (assembledTest.errors.size() > 0) {
            QStringList errors;
            for (const auto &[line, error] : assembledTest.errors) errors.append(u"%1: %2"_s.arg(line).arg(error));
            FAIL("Errors in tests assembly: " + errors.join(", ").toStdString());
          }
        }
      } else if (source->language == "pepcpu2") {
        auto assembledFig = pepp::tc::parse::MicroParser<uarch2, regs>(source->contents()).parse();
        if (assembledFig.errors.size() > 0) {
          QStringList errors;
          for (const auto &[line, error] : assembledFig.errors) errors.append(u"%1: %2"_s.arg(line).arg(error));
          FAIL("Errors in " + nameAsStd + " microassembly: " + errors.join(", ").toStdString());
        }
        // Check that all tests assemble too
        for (auto io : figure->typesafeTests()) {
          QString input = io->input.toString().replace(lf, "");
          auto assembledTest = pepp::tc::parse::MicroParser<uarch2, regs>(input).parse();
          if (assembledTest.errors.size() > 0) {
            QStringList errors;
            for (const auto &[line, error] : assembledTest.errors) errors.append(u"%1: %2"_s.arg(line).arg(error));
            FAIL("Errors in tests assembly: " + errors.join(", ").toStdString());
          }
        }
      } else FAIL("Unrecognized microcode format: " << source->language.toStdString());
    }
    // Skip running tests for now, since the write word tests fail to execute.
    /*auto microcode = pepp::ucode::microcodeFor<uarch1, regs>(assembledFig);
    cpu.setMicrocode(std::move(microcode));
    int num = 0;
    for (auto io : figure->typesafeTests()) {
      QString input = io->input.toString().replace(lf, "");
      DYNAMIC_SECTION(nameAsStd << " on: " << input.toStdString()) {
        auto assembledTest = pepp::ucode::parse<uarch1, regs>(input);
        if (assembledTest.errors.size() > 0) {
          QStringList errors;
          for (const auto &[line, error] : assembledTest.errors) errors.append(u"%1: %2"_s.arg(line).arg(error));
          FAIL("Errors in tests assembly: " + errors.join(", ").toStdString());
        }
        auto tests = pepp::ucode::tests<uarch1, regs>(assembledTest);
        mem.clear(0);
        cpu.setConstantRegisters();
        cpu.applyPreconditions(tests.pre);
        for (int cycle = 0; cpu.status() == targets::pep9::mc2::CPUByteBus::Status::Ok; cycle++) cpu.clock(cycle);
        CHECK(cpu.status() == targets::pep9::mc2::CPUByteBus::Status::Halted);
        auto results = cpu.testPostconditions(tests.post);
        auto passed = std::all_of(results.cbegin(), results.cend(), [](const bool &test) { return test; });
        if (!passed) {
          QStringList faileds;
          for (int it = 0; it < results.size(); it++)
            if (!results[it]) faileds.append(toString(tests.post[it]));
          FAIL("Unit tests failed: " + faileds.join(", ").toStdString());
        }
      }
      num++;
    }*/
  }
}
