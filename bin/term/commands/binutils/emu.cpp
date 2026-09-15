#include "emu.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

PeppEmulator::PeppEmulator(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

void PeppEmulator::run() {
  std::unique_ptr<System> system;
  if (std::holds_alternative<std::string>(_opts.system))
    throw std::runtime_error("Custom system JSON not yet supported");
  else {
    switch (std::get<SystemEnu>(_opts.system)) {
    case SystemEnu::RV32I: throw std::runtime_error("RV32I system not yet supported");
    case SystemEnu::Pep10OS: system = create_standard_pep10_system(); break;
    case SystemEnu::Pep10BM: system = create_standard_pep10_system(); break;
    }
  }
  if (system == nullptr) throw std::runtime_error("Failed to create system");
  system->initialize();
  std::cout << "system: " << system->config().compatible << std::endl;
  return emit finished(0);
}
