#include "as.hpp"
#include <iostream>

AsTask::AsTask(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void AsTask::run() {
  switch (_opts.arch) {
  case pepp::Architecture::NO_ARCH:
    std::cerr << "Error: No architecture specified. Use -march to specify an architecture.\n";
    return emit finished(1);
  case pepp::Architecture::PEP8: assemble_pep(); break;
  case pepp::Architecture::PEP9: assemble_pep(); break;
  case pepp::Architecture::PEP10: assemble_pep(); break;
  case pepp::Architecture::RISCV: assemble_riscv(); break;
  }

  return emit finished(0);
}

void AsTask::assemble_riscv() { std::cerr << "Assembling for RV32I...\n"; }

void AsTask::assemble_pep() { std::cerr << "Assembling for PEP architecture...\n"; }
