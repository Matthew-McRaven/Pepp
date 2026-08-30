/*
 * Copyright (c) 2023-2026 J. Stanley Warford, Matthew McRaven
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

#include <QtCore>
#include <chrono>
#include "../shared.hpp"
#include "../task.hpp"
#include "CLI11.hpp"
#include "core/integers.h"

class ThroughputTask : public Task {
  Q_OBJECT
public:
  enum class WhichVersion { Sim3, Core, RV };
  enum class TestProgram {
    SelfBranch, // self: BR self
    RMW,        // Accumulate a meaningless value into A.
  };
  ThroughputTask(WhichVersion ver, QObject *parent = nullptr);
  ~ThroughputTask() = default;
  void run();

  // Both should return their "start" time
  std::chrono::high_resolution_clock::time_point do_sim3();
  std::chrono::high_resolution_clock::time_point do_core();
  std::chrono::high_resolution_clock::time_point do_riscv();
  u64 maxInstr = 100'000'000;
  bool has_bps = false;
  bool use_sparse = false;
  TestProgram program = TestProgram::SelfBranch;

private:
  std::vector<u8> pep_program(TestProgram prog) const;
  std::vector<u8> rv_program(TestProgram prog) const;
  WhichVersion _version;
};

void registerThroughput(auto &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto instrThruSC = app.add_subcommand("mit", "Measure instruction throughput");
  static ThroughputTask::WhichVersion version = ThroughputTask::WhichVersion::Core;
  static ThroughputTask::TestProgram program = ThroughputTask::TestProgram::SelfBranch;
  static u64 maxInstr = 100'000'000;
  static bool has_bps = false, use_sparse = false;
  auto versionOpt = instrThruSC->add_option("-v,--version", version, "Which version to run")
                        ->transform(CLI::CheckedTransformer(std::map<std::string, ThroughputTask::WhichVersion>{
                            {"sim3", ThroughputTask::WhichVersion::Sim3},
                            {"core", ThroughputTask::WhichVersion::Core},
                            {"rv", ThroughputTask::WhichVersion::RV}}));
  auto programOpt =
      instrThruSC->add_option("-p,--program", program, "Which test program to run")
          ->transform(CLI::CheckedTransformer(std::map<std::string, ThroughputTask::TestProgram>{
              {"br", ThroughputTask::TestProgram::SelfBranch}, {"rmw", ThroughputTask::TestProgram::RMW}}));
  static auto maxInstrOpt =
      instrThruSC->add_option("-n,--max-instr", maxInstr, "Maximum number of instructions to run");
  static auto hasBpsOpt =
      instrThruSC->add_flag("--bps,!--no-bps", has_bps, "Add spurious breakpoints which will not be hit")
          ->default_val(false);
  static auto useSparseOpt =
      instrThruSC->add_flag("--sparse,!--no-sparse", use_sparse, "Use Sparse storage for RAM rather then Dense")
          ->default_val(false);
  instrThruSC->group("");
  instrThruSC->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    task = [](QObject *parent) {
      auto ret = new ThroughputTask(version, parent);
      ret->maxInstr = maxInstr;
      ret->has_bps = has_bps;
      ret->program = program;
      ret->use_sparse = use_sparse;
      return ret;
    };
  });
}
