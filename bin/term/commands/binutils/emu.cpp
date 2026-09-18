#include "emu.hpp"
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "core/formats/elf/packed_input_group.hpp"
#include "core/formats/elf/packed_io.hpp"
#include "core/sim/api/loadable.hpp"
#include "core/sim/cores/cpu/pep/pep_isa.hpp"
#include "core/sim/debugger/register_scanner.hpp"
#include "core/sim/devicetree.hpp"
#include "core/sim/loader.hpp"
#include "core/sim/memory/io/fifo.hpp"
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

// Resolve a potentially ambiguous device name, returning nullptr if there is not exactly 1 match.
Device *find_device(System &system, const std::string &name) {
  const auto matches = system.find_all(name);
  if (matches.size() == 1) return matches.front();
  else if (matches.empty()) std::cerr << "Error: No device named " << name << "\n";
  else {
    std::cerr << "Error: More than one device is named " << name << ". Choose from:";
    for (const auto *dev : matches) std::cerr << " " << dev->config().fullname;
    std::cerr << "\n";
  }
  return nullptr;
}

FIFORegister *find_fifo(System &system, const std::string &name) {
  auto *dev = find_device(system, name);
  if (dev == nullptr) return nullptr;
  auto *fifo = dynamic_cast<FIFORegister *>(dev);
  if (fifo == nullptr) std::cerr << "Error: " << name << " is not a memory-mapped FIFO\n";
  return fifo;
}

// Look up a register or field by name, reporting an error unless there is exactly one match.
std::optional<RegisterScan::RegisterRef> lookup(RegisterScan &scan, const std::string &name) {
  const auto ref = scan.find(name);
  if (!ref) std::cerr << "Error: No unique register or field named " << name << "\n";
  return ref;
}
} // namespace

PeppEmulator::PeppEmulator(Options &opts, QObject *parent) : Task(parent), _opts(opts) {}

int PeppEmulator::do_load(System &system) {
  // Which device an unnamed file loads into, and what to suggest when there is more than one candidate.
  std::vector<Device *> loadables;
  for (auto *dev : *system.root())
    if (dev->capability<Loadable>() != nullptr) loadables.push_back(dev);

  Loader loader(&system);
  for (const auto &[device, file] : _opts.elf_files) {
    Device *dest = nullptr;
    if (!device.empty()) {
      if (dest = find_device(system, device); dest == nullptr) return 1;
      else if (dest->capability<Loadable>() == nullptr) {
        std::cerr << "Error: " << device << " cannot be loaded into\n";
        return 1;
      }
    } else if (loadables.size() == 1) dest = loadables.front();
    else if (loadables.empty()) {
      std::cerr << "Error: The system has no loadable device to load " << file << " into\n";
      return 1;
    } else {
      std::cerr << "Error: The system has more than one loadable device, so " << file
                << " must name one with <device>=. Choose from:";
      for (const auto *dev : loadables) std::cerr << " " << dev->config().fullname;
      std::cerr << "\n";
      return 1;
    }

    try {
      loader.add_group(pepp::bts::to_input_group(pepp::bts::open_input_elf(file)), dest->id());
    } catch (const std::exception &e) {
      std::cerr << "Error: Could not load " << file << ": " << e.what() << "\n";
      return 1;
    }
  }

  // Cores may depend on memory being initialized (e.g., Pep/10s memory vectors), so perform per-core init after program
  // load.
  try {
    for (auto *dev : *system.root())
      if (auto *loadable = dev->capability<Loadable>()) loadable->register_core_init(loader);
  } catch (const std::exception &e) {
    std::cerr << "Error: Could not initialize cores: " << e.what() << "\n";
    return 1;
  }

  // Inject command-line-provided register values. Last so they can override any per-core init.
  auto *scan = system.register_scan();
  for (const auto &[name, value] : _opts.set_registers) {
    const auto ref = lookup(*scan, name);
    if (!ref) return 1;
    else if (const auto width = scan->bit_width(*ref); !fits(value, width)) {
      std::cerr << "Error: Value for " << name << " does not fit in " << int(width) << " bits\n";
      return 1;
    } else if (!loader.set_register(*ref, value)) {
      std::cerr << "Error: Could not set " << name << "\n";
      return 1;
    }
  }

  if (!loader.run()) {
    std::cerr << "Error: Loading failed with stop cause " << static_cast<int>(loader.stop_cause()) << "\n";
    return 1;
  }
  return 0;
}

int PeppEmulator::do_input(System &system) {
  for (const auto &[device, file] : _opts.mmi) {
    auto *fifo = find_fifo(system, device);
    if (fifo == nullptr) return 1;
    std::ostringstream buffer;
    if (file == "-") buffer << std::cin.rdbuf();
    else if (std::ifstream in(file, std::ios::binary); !in) {
      std::cerr << "Error: Could not open " << file << " for reading\n";
      return 1;
    } else if (buffer << in.rdbuf(); in.bad()) {
      std::cerr << "Error: Could not read " << file << "\n";
      return 1;
    }
    const auto bytes = buffer.str();
    for (const auto byte : bytes) fifo->input().push(static_cast<u8>(byte));
  }
  return 0;
}

int PeppEmulator::do_output(System &system) {
  for (const auto &[device, file] : _opts.mmo) {
    auto *fifo = find_fifo(system, device);
    if (fifo == nullptr) return 1;
    std::string bytes;
    for (auto it = fifo->output().begin(); it != fifo->output().end(); ++it) bytes.push_back(static_cast<char>(*it));
    if (file == "-") {
      std::cout << bytes;
      if (!std::cout.flush()) {
        std::cerr << "Error: Could not write " << device << " to stdout\n";
        return 1;
      }
    } else if (std::ofstream out(file, std::ios::binary | std::ios::trunc); !out) {
      std::cerr << "Error: Could not open " << file << " for writing\n";
      return 1;
    } else if (out.write(bytes.data(), bytes.size()); !out.flush()) {
      std::cerr << "Error: Could not write " << device << " to " << file << "\n";
      return 1;
    }
  }
  return 0;
}

int PeppEmulator::do_run(System &system) {
  // Clock the Pep CPU directly until the program powers off, because I have not implemented the clock tree.
  PepISA3CPU *cpu = nullptr;
  for (auto *dev : *system.root())
    if (auto *as_cpu = dynamic_cast<PepISA3CPU *>(dev); as_cpu != nullptr) cpu = as_cpu;
  if (cpu == nullptr) {
    std::cerr << "Error: The system has no Pep CPU to run\n";
    return 1;
  }
  auto *pwr_off = dynamic_cast<FIFORegister *>(system.find_absolute("/bus/pwrOff"));
  if (pwr_off == nullptr) {
    std::cerr << "Error: The system has no /bus/pwrOff to stop on\n";
    return 1;
  }
  try {
    for (u64 tick = 0; pwr_off->output().empty(); ++tick) cpu->clock_tick(PulseSchedule::PulseIndex{tick}, tick);
  } catch (const std::exception &e) {
    std::cerr << "Error: Simulation stopped: " << e.what() << "\n";
    return 1;
  }
  return 0;
}

int PeppEmulator::do_print(System &system) {
  auto *scan = system.register_scan();
  for (const auto &name : _opts.print_registers) { // Printed in the same format of --set-reg.
    const auto ref = lookup(*scan, name);
    if (!ref) return 1;
    try {
      const auto value = scan->read<u64>(*ref);
      std::cout << fmt::format("{}=0x{:0{}X}\n", name, value, (scan->bit_width(*ref) + 3) / 4);
    } catch (const std::runtime_error &e) {
      std::cerr << "Error: Could not read " << name << ": " << e.what() << "\n";
      return 1;
    }
  }
  return 0;
}

void PeppEmulator::run() {
  std::unique_ptr<System> system;
  if (std::holds_alternative<std::string>(_opts.system))
    throw std::runtime_error("Custom system JSON not yet supported");
  else {
    switch (std::get<SystemEnu>(_opts.system)) {
    case SystemEnu::RV32I: throw std::runtime_error("RV32I system not yet supported");
    case SystemEnu::Pep10: system = create_standard_pep10_system(); break;
    }
  }
  if (system == nullptr) throw std::runtime_error("Failed to create system");
  system->initialize();

  if (const auto code = do_load(*system); code != 0) return emit finished(code);
  else if (const auto code = do_input(*system); code != 0) return emit finished(code);
  else if (const auto code = do_run(*system); code != 0) return emit finished(code);
  else if (const auto code = do_output(*system); code != 0) return emit finished(code);
  else if (const auto code = do_print(*system); code != 0) return emit finished(code);

  return emit finished(0);
}
