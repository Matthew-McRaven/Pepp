#include "rvemu.hpp"
#include <CLI11.hpp>
#include <chrono>
#include <fstream>
#include <inttypes.h>
#include <limits>
#include <sim3/systems/notraced_riscv_isa3_system/debug.hpp>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include "sim3/systems/notraced_riscv_isa3_system.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/debug.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/rsp_server.hpp"
#if __has_include(<unistd.h>)
#include <fcntl.h>
#endif
#if defined(__linux__)
#include <sys/mman.h>
#include <sys/stat.h>
#endif

static constexpr bool full_linux_guest = true;
static constexpr bool newlib_mini_guest = true;
static constexpr bool micro_guest = true;

static inline std::vector<uint8_t> load_file(const std::string &);

static const std::string DYNAMIC_LINKER = "/usr/riscv64-linux-gnu/lib/ld-linux-riscv64-lp64d.so.1";
// #define NODEJS_WORKAROUND

template <riscv::AddressType address_t> static void run_sighandler(riscv::Machine<address_t> &);

template <riscv::AddressType address_t>
static void run_program(const RVEmuTask::Arguments &cli_args, const std::string_view binary, const bool is_dynamic,
                        const std::vector<std::string> &args) {
  auto options = std::make_shared<riscv::MachineOptions<address_t>>(riscv::MachineOptions<address_t>{
      .memory_max = cli_args.max_memory,
      .enforce_exec_only = cli_args.execute_only,
      .ignore_text_section = cli_args.ignore_text,
      .verbose_loader = cli_args.verbose,
      .use_shared_execute_segments =
          false, // We are only creating one machine, disabling this can enable some optimizations
#ifdef NODEJS_WORKAROUND
      .ebreak_locations =
          {
              "pthread_rwlock_rdlock", "pthread_rwlock_wrlock" // Live-patch locations
          },
#endif
  });

  // Create a RISC-V machine with the binary as input program
  auto st0 = std::chrono::high_resolution_clock::now();
  riscv::Machine<address_t> machine{binary, *options};
  if (cli_args.verbose) {
    auto st1 = std::chrono::high_resolution_clock::now();
    printf("* Loaded in %.3f ms\n", std::chrono::duration<double, std::milli>(st1 - st0).count());
  }

  // Remember the options for later in case background compilation is enabled,
  // if new execute segments need to be decoded and so on. Basically all future
  // operations that need to know the options. This is optional.
  machine.set_options(std::move(options));

  if (cli_args.quit) return; // Quit after instantiating the machine

  // A helper system call to ask for symbols that is possibly only known at runtime
  // Used by testing executables
  address_t symbol_function = 0;
  machine.set_userdata(&symbol_function);
  machine.install_syscall_handler(500, [](auto &machine) {
    auto [addr] = machine.template sysargs<address_t>();
    auto &symfunc = *machine.template get_userdata<decltype(symbol_function)>();
    symfunc = addr;
    printf("Introduced to symbol function: 0x%" PRIX64 "\n", uint64_t(addr));
  });

  if (full_linux_guest) {
    std::vector<std::string> env = {"LC_CTYPE=C", "LC_ALL=C", "RUST_BACKTRACE=full"};
    machine.setup_linux(args, env);
    // Linux system to open files and access internet
    machine.setup_linux_syscalls();
    machine.fds().permit_filesystem = !cli_args.sandbox;
    machine.fds().permit_sockets = !cli_args.sandbox;
    if (!cli_args.sandbox) {
      // Enable stdin when sandbox is disabled.
      machine.set_stdin(
          [](const riscv::Machine<address_t> &machine, char *buf, size_t size) -> long { return read(0, buf, size); });
      machine.fds().proxy_mode = true; // Proxy mode for system calls (no more sandbox)
#ifndef _WIN32
      char buf[4096];
      machine.fds().cwd = getcwd(buf, sizeof(buf));
#endif
    }
    // Rewrite certain links to masquerade and simplify some interactions (eg. /proc/self/exe)
    machine.fds().filter_readlink = [&](void *user, std::string &path) {
      if (path == "/proc/self/exe") {
        path = machine.fds().cwd + "/program";
        return true;
      }
      if (path == "/proc/self/fd/1" || path == "/proc/self/fd/2") {
        return true;
      }
      fprintf(stderr, "Guest wanted to readlink: %s (denied)\n", path.c_str());
      return false;
    };
    // Only allow opening certain file paths. The void* argument is
    // the user-provided pointer set in the RISC-V machine.
    machine.fds().filter_open = [=](void *user, std::string &path) {
      (void)user;
      if (path == "/etc/hostname" || path == "/etc/hosts" || path == "/etc/nsswitch.conf" || path == "/etc/host.conf" ||
          path == "/etc/resolv.conf")
        return true;
      if (path == "/dev/urandom") return true;
      if (path == "/program") { // Fake program path
        path = args.at(0);      // Sneakily open the real program instead
        return true;
      }
      if (path == "/etc/ssl/certs/ca-certificates.crt") return true;

      // Paths that are allowed to be opened
      static const std::string sandbox_libdir = "/lib/riscv64-linux-gnu/";
      // The real path to the libraries (on the host system)
      static const std::string real_libdir = "/usr/riscv64-linux-gnu/lib/";
      // The dynamic linker and libraries we allow
      static const std::vector<std::string> libs = {"libdl.so.2",     "libm.so.6",       "libgcc_s.so.1",
                                                    "libc.so.6",      "libatomic.so.1",  "libstdc++.so.6",
                                                    "libresolv.so.2", "libnss_dns.so.2", "libnss_files.so.2"};

      if (path.find(sandbox_libdir) == 0) {
        // Find the library name
        auto lib = path.substr(sandbox_libdir.size());
        if (std::find(libs.begin(), libs.end(), lib) == libs.end()) {
          if (cli_args.verbose) {
            fprintf(stderr, "Guest wanted to open: %s (denied)\n", path.c_str());
          }
          return false;
        } else if (cli_args.verbose) {
          fprintf(stderr, "Guest wanted to open: %s (allowed)\n", path.c_str());
        }
        // Construct new path
        path = real_libdir + path.substr(sandbox_libdir.size());
        return true;
      }

      if (is_dynamic && args.size() > 1 && path == args.at(1)) {
        return true;
      }
      if (!cli_args.sandbox) {
        return true;
      }
      for (const auto &allowed : cli_args.allowed_files) {
        if (path == allowed) {
          return true;
        }
      }
      if (cli_args.verbose) {
        fprintf(stderr, "Guest wanted to open: %s (denied)\n", path.c_str());
      }
      return false;
    };
    // multi-threading
    machine.setup_posix_threads();
  } else if (newlib_mini_guest) {
    // the minimum number of syscalls needed for malloc and C++ exceptions
    machine.setup_newlib_syscalls(true);
    machine.fds().permit_filesystem = !cli_args.sandbox;
    machine.setup_argv(args);
    machine.on_unhandled_syscall = [](riscv::Machine<address_t> &machine, size_t num) {
      if (num == 1024) { // newlib_open()
        const auto g_path = machine.sysarg(0);
        const int flags = machine.template sysarg<int>(1);
        const int mode = machine.template sysarg<int>(2);
        // This is a custom syscall for the newlib mini guest
        std::string path = machine.memory.memstring(g_path);
        if (machine.has_file_descriptors() && machine.fds().permit_filesystem) {

          if (machine.fds().filter_open != nullptr) {
            // filter_open() can modify the path
            if (!machine.fds().filter_open(machine.template get_userdata<void>(), path)) {
              machine.set_result(-EPERM);
              return;
            }

#if !defined(_MSC_VER)
            int res = open(path.c_str(), flags, mode);
            if (res > 0) res = machine.fds().assign_file(res);
            machine.set_result_or_error(res);
            return;
#endif
          }
        }
        machine.set_result(-EPERM);
        return;
      }
      fprintf(stderr, "Unhandled syscall: %zu\n", num);
    };
    machine.fds().filter_open = [=](void *, std::string &path) {
      if (!cli_args.sandbox) return true;

      for (const auto &allowed : cli_args.allowed_files) {
        if (path == allowed) return true;
      }
      if (cli_args.verbose) {
        fprintf(stderr, "Guest wanted to open: %s (denied)\n", path.c_str());
      }
      return false;
    };
  } else if (micro_guest) {
    // This guest has accelerated libc functions, which
    // are provided as system calls
    // See: tests/unit/native.cpp and tests/unit/include/native_libc.h
    constexpr size_t heap_size = 6ULL << 20; // 6MB
    auto heap = machine.memory.mmap_allocate(heap_size);

    machine.setup_native_heap(470, heap, heap_size);
    machine.setup_native_memory(475);
    machine.setup_native_threads(490);

    machine.setup_newlib_syscalls();
    machine.setup_argv(args);
  } else {
    fprintf(stderr, "Unknown emulation mode! Exiting...\n");
    exit(1);
  }

  // A CLI debugger used with --debug or DEBUG=1
  riscv::DebugMachine debug{machine};

  if (cli_args.debug) {
    // Print all instructions by default
    const bool vi = true;
    // With --verbose we also print register values after
    // every instruction.
    const bool vr = cli_args.verbose;

    auto main_address = machine.address_of("main");
    if (cli_args.from_start || main_address == 0x0) {
      debug.verbose_instructions = vi;
      debug.verbose_registers = vr;
      // Without main() this is a custom or stripped program,
      // so we break immediately.
      debug.print_and_pause();
    } else {
      // Automatic breakpoint at main() to help debug certain programs
      debug.breakpoint(main_address, [vi, vr](auto &debug) {
        auto &cpu = debug.machine.cpu;
        // Remove the breakpoint to speed up debugging
        debug.erase_breakpoint(cpu.pc());
        debug.verbose_instructions = vi;
        debug.verbose_registers = vr;
        printf("\n*\n* Entered main() @ 0x%" PRIX64 "\n*\n", uint64_t(cpu.pc()));
        debug.print_and_pause();
      });
    }
  }

  auto t0 = std::chrono::high_resolution_clock::now();
  try {
    // If you run the emulator with --gdb or GDB=1, you can connect
    // with gdb-multiarch using target remote localhost:2159.
    if (cli_args.debug) {
      printf("GDB server is listening on localhost:2159\n");
      riscv::RSP<address_t> server{machine, 2159};
      auto client = server.accept();
      if (client != nullptr) {
        printf("GDB is connected\n");
        while (client->process_one());
      }
      if (!machine.stopped()) {
        // Run remainder of program
        machine.simulate(cli_args.fuel);
      }
    } else if (cli_args.debug) {
      // CLI debug simulation
      debug.simulate();
    } else if (cli_args.accurate) {
      // Single-step precise simulation
      machine.set_max_instructions(~0ULL);
      machine.cpu.simulate_precise();
    } else {
#ifdef NODEJS_WORKAROUND
      // In order to get NodeJS to work we need to live-patch deadlocked rwlocks
      // This is a temporary workaround until the issue is found and fixed.
      static const auto rw_rdlock = machine.address_of("pthread_rwlock_rdlock");
      static const auto rw_wrlock = machine.address_of("pthread_rwlock_wrlock");
      machine.install_syscall_handler(riscv::SYSCALL_EBREAK, [](auto &machine) {
        auto &cpu = machine.cpu;
        if (cpu.pc() == rw_rdlock || cpu.pc() == rw_wrlock) {
          // Execute 2 instruction and step over them
          cpu.step_one(false);
          cpu.step_one(false);
          // Check for deadlock
          if (cpu.reg(14) == cpu.reg(15)) {
            // Deadlock detected, avoid branch (beq a4, a5) and reset the lock
            cpu.reg(14) = 0xFF;
            machine.memory.template write<uint32_t>(cpu.reg(10), 0);
          }
        } else {
          throw riscv::MachineException(riscv::UNHANDLED_SYSCALL, "EBREAK instruction", cpu.pc());
        }
      });
#endif // NODEJS_WORKAROUND

      // Normal RISC-V simulation
      if (cli_args.accurate) machine.simulate(cli_args.fuel);
      else {
        // Simulate until it eventually stops (or user interrupts)
        machine.simulate(std::numeric_limits<uint64_t>::max());
      }
    }
  } catch (const riscv::MachineException &me) {
    printf("%s\n", machine.cpu.current_instruction_to_string().c_str());
    printf(">>> Machine exception %d: %s (data: 0x%" PRIX64 ")\n", me.type(), me.what(), me.data());
    printf("%s\n", machine.cpu.registers().to_string().c_str());
    machine.memory.print_backtrace([](std::string_view line) { printf("-> %.*s\n", (int)line.size(), line.begin()); });
    if (me.type() == riscv::UNIMPLEMENTED_INSTRUCTION || me.type() == riscv::MISALIGNED_INSTRUCTION) {
      printf(">>> Is an instruction extension disabled?\n");
      printf(">>> A-extension: %d  C-extension: %d  V-extension: %d\n", riscv::atomics_enabled,
             riscv::compressed_enabled, riscv::vector_extension);
    }
    if (cli_args.debug) debug.print_and_pause();
    else run_sighandler(machine);
  } catch (std::exception &e) {
    printf(">>> Exception: %s\n", e.what());
    machine.memory.print_backtrace([](std::string_view line) { printf("-> %.*s\n", (int)line.size(), line.begin()); });
    if (cli_args.debug) debug.print_and_pause();
    else run_sighandler(machine);
  }

  auto t1 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> runtime = t1 - t0;

  if (!cli_args.silent) {
    const auto retval = machine.return_value();
    printf(">>> Program exited, exit code = %" PRId64 " (0x%" PRIX64 ")\n", int64_t(retval), uint64_t(retval));
    if (cli_args.accurate)
      printf("Instructions executed: %" PRIu64 "  Runtime: %.3fms  Insn/s: %.0fmi/s\n", machine.instruction_counter(),
             runtime.count() * 1000.0, machine.instruction_counter() / (runtime.count() * 1e6));
    else printf("Runtime: %.3fms   (Use --accurate for instruction counting)\n", runtime.count() * 1000.0);
    printf("Pages in use: %zu (%" PRIu64 " kB virtual memory, total %" PRIu64 " kB)\n", machine.memory.pages_active(),
           machine.memory.pages_active() * riscv::Page::size() / uint64_t(1024),
           machine.memory.memory_usage_total() / uint64_t(1024));
  }

  if (!cli_args.call_function.empty()) {
    auto addr = machine.address_of(cli_args.call_function);
    if (addr == 0 && symbol_function != 0) {
      addr = machine.vmcall(symbol_function, cli_args.call_function);
    }
    if (addr != 0) {
      printf("Calling function %s @ 0x%lX\n", cli_args.call_function.c_str(), long(addr));
      machine.vmcall(addr);
    } else {
      printf("Error: Function %s not found, not able to call\n", cli_args.call_function.c_str());
    }
  }
}

template <riscv::AddressType address_t> void run_sighandler(riscv::Machine<address_t> &machine) {
  constexpr int SIG_SEGV = 11;
  auto &action = machine.sigaction(SIG_SEGV);
  if (action.is_unset()) return;

  auto handler = action.handler;
  action.handler = 0x0; // Avoid re-triggering(?)

  machine.stack_push(machine.cpu.reg(riscv::REG_RA));
  machine.cpu.reg(riscv::REG_RA) = machine.cpu.pc();
  machine.cpu.reg(riscv::REG_ARG0) = 11; /* SIGSEGV */
  try {
    machine.cpu.jump(handler);
    machine.simulate(60'000);
  } catch (...) {
  }

  action.handler = handler;
}

std::vector<uint8_t> load_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file.is_open()) throw std::runtime_error("Could not open file: " + filename);

  return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

RVEmuTask::RVEmuTask(const Arguments args, std::string fname, QObject *parent)
    : Task(parent), args(args), fname(fname) {}

void RVEmuTask::run() {

  using ElfHeader = typename riscv::Elf<uint32_t>::Header;
  std::vector<std::string> prog_args = args.prog_args;

  try {
    std::vector<uint8_t> vbin;
#if !defined(__linux__)
    // Use load_file for non-Posix systems
    vbin = load_file(this->fname);
    if (vbin.size() < sizeof(ElfHeader)) {
      fprintf(stderr, "ELF binary was too small to be usable!\n");
      exit(1);
    }
    std::string_view binary{(const char *)vbin.data(), vbin.size()};
#else
    // Use mmap for Posix systems, not sure if Apple supports this
    std::string_view binary;
    int fd = open(fname.c_str(), O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "Could not open file: %s\n", fname.c_str());
      exit(1);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
      fprintf(stderr, "Could not stat file: %s\n", fname.c_str());
      exit(1);
    }
    if (st.st_size < sizeof(ElfHeader)) {
      fprintf(stderr, "ELF binary was too small to be usable!\n");
      exit(1);
    }
    void *ptr = mmap(nullptr, st.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE | MAP_NORESERVE, fd, 0);
    if (ptr == MAP_FAILED) {
      fprintf(stderr, "Could not mmap file: %s\n", fname.c_str());
      exit(1);
    }
    binary = {(const char *)ptr, size_t(st.st_size)};
#endif

    bool is_dynamic = false;
    if (binary[4] == riscv::ELFCLASS64) {
      std::string_view interpreter;
      std::tie(is_dynamic, interpreter) =
          riscv::Elf<uint64_t>::is_dynamic(std::string_view(binary.data(), binary.size()));

      /*if (is_dynamic) {
        // Load the dynamic linker shared object
        if (interpreter.empty()) {
          interpreter = DYNAMIC_LINKER;
        }
        try {
          vbin = load_file(std::string(interpreter));
        } catch (const std::exception &e) {
          vbin = load_file(DYNAMIC_LINKER);
        }
        binary = {(const char *)vbin.data(), vbin.size()};
        // Insert program name as argv[1]
        prog_args.insert(prog_args.begin() + 1, prog_args.at(0));
        // Set dynamic linker to argv[0]
        prog_args.at(0) = interpreter;
      }*/
    }

    if (binary[4] == riscv::ELFCLASS64) run_program<uint64_t>(this->args, binary, is_dynamic, prog_args);
    else if (binary[4] == riscv::ELFCLASS32) run_program<uint32_t>(this->args, binary, is_dynamic, prog_args);
    else {
      std::cerr << fmt::format("Unknown ELF class {}", binary[4]);
      return emit finished(1);
    }
  } catch (const std::exception &e) {
    std::cerr << fmt::format("Exception: {}", e.what());
    return emit finished(2);
  }
  return emit finished(0);
}

void register_rvemu(CLI::App &app, task_factory_t &task, detail::SharedFlags &flags) {
  static auto rvemu = app.add_subcommand("rvemu", "Emulate a user-mode RISC-V program");
  static RVEmuTask::Arguments args;
  static std::string prog_str;
  static auto verbose_flag = rvemu->add_flag("-v,--verbose", args.verbose, "Enable verbose loader output");
  static auto silent_flag = rvemu->add_flag("-s,--silent", args.silent, "Suppress program completion information");
  static auto prog = rvemu->add_option("program", prog_str, "ELF file to load and simulate")->required(true);

  static auto dbg = rvemu->add_option_group("Guest Debugging", "");
  static auto debug_flag =
      dbg->add_flag("-d,--debug", args.debug, "Enable CLI debugger and start the gdb server on 2159.");
  static auto fromstart_flag =
      dbg->add_flag("-F,--from-start", args.from_start, "Start debugger from the beginning (_start)");
  static auto trace_flag = dbg->add_flag("-T,--trace", args.instr_trace, "Print an instruction trace to stdout");

  static auto config = rvemu->add_option_group("Simulation", "Control simulator resource usage and simulation");
  static auto accurate_flag =
      rvemu->add_flag("-a,--accurate", args.accurate, "Accurate instruction counting with precise exceptions.");
  static auto fuel_opt = rvemu->add_option("-f,--fuel", args.fuel, "Set max instructions until program halts");
  static auto memory_opt =
      rvemu->add_option("-m,--memory", args.max_memory, "Set max memory size in MiB (default: 4096 MiB)");

  static auto sandbox = rvemu->add_option_group("Sandboxing", "Control the permissions of simulated programs");
  static auto sandbox_flag = sandbox->add_flag("--sandbox,!--no-sandbox",
                                               "Enable or disable strict emulator sandbox. Sockets are not usable in "
                                               "the sandbox, and only allowed files (via -A) are exposed");
  static auto allowed_opt =
      sandbox->add_option("-A,--allow", args.allowed_files, "Allow file to be opened by the guest");
  static auto excute_only_flag =
      sandbox->add_flag("-X,--execute-only", args.execute_only, "Enforce execute-only segments (no read/write)");

  static auto loader = rvemu->add_option_group("Loader", "Modify ELF loading");
  static auto ignore_flag =
      loader->add_flag("-I,--ignore-text", args.ignore_text, "Ignore .text section, and use segments only");
  static auto call_opt =
      loader->add_option("-c,--call", args.call_function, "Call a function after loading the program");
  static auto quit_flag = loader->add_flag("-Q,--quit", args.quit, "Quit after loading the program");
  rvemu->callback([&]() {
    flags.kind = detail::SharedFlags::Kind::TERM;
    if (*trace_flag && !args.accurate) {
      std::cerr << "--trace requires --accurate";
      QCoreApplication::exit(1);
    } else task = [&](QObject *parent) { return new RVEmuTask(args, prog_str, parent); };
  });
}
