#include "emu.hpp"
#include <fmt/format.h>
#include <iostream>
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/system.hpp"
#include "core/sim/systemparser.hpp"

namespace {
// True if value, possibly a negative number in two's complement, can be stored in width bits without losing information.
bool fits(u64 value, u8 width) {
  if (width == 0) return value == 0;
  else if (width >= 64 || value >> width == 0) return true;
  const auto as_signed = static_cast<i64>(value);
  return as_signed < 0 && as_signed >= -(i64(1) << (width - 1));
}
} // namespace

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

  auto scan = system->register_scan();
  // Look up a register or field by name, reporting an error unless there is exactly one match.
  auto lookup = [&](const std::string &name) {
    const auto ref = scan->find(name);
    if (!ref) std::cerr << "Error: No unique register or field named " << name << "\n";
    return ref;
  };

  // Values from --set-reg override whatever the system chose during initialization.
  for (const auto &[name, value] : _opts.set_registers) {
    const auto ref = lookup(name);
    if (!ref) return emit finished(1);
    else if (const auto width = scan->bit_width(*ref); !fits(value, width)) {
      std::cerr << "Error: Value for " << name << " does not fit in " << int(width) << " bits\n";
      return emit finished(1);
    }
    try {
      scan->write<u64>(*ref, value);
    } catch (const std::runtime_error &e) {
      std::cerr << "Error: Could not set " << name << ": " << e.what() << "\n";
      return emit finished(1);
    }
  }

  // TODO: actually load the program into the simulator and run it.

  for (const auto &name : _opts.print_registers) { // Printed in the same format accepted by --set-reg.
    const auto ref = lookup(name);
    if (!ref) return emit finished(1);
    try {
      const auto value = scan->read<u64>(*ref);
      std::cout << fmt::format("{}=0x{:0{}X}\n", name, value, (scan->bit_width(*ref) + 3) / 4);
    } catch (const std::runtime_error &e) {
      std::cerr << "Error: Could not read " << name << ": " << e.what() << "\n";
      return emit finished(1);
    }
  }
  return emit finished(0);
}
