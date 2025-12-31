#include <QCoreApplication>
#include <QDirIterator>
#include <any>
#include <catch.hpp>
#include "loader.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"
#include "sim3/systems/notraced_riscv_isa3_system/debug.hpp"

static const uint64_t MAX_MEMORY = 8ul << 20; /* 8MB */
static const uint64_t MAX_INSTRUCTIONS = 10'000'000ul;

TEST_CASE("Instantiate machine", "[Instantiate]") {
  // for (QDirIterator i("://", QDirIterator::Subdirectories); i.hasNext();) qDebug() << i.next();

  const auto binary = load("://freestanding/basic_a.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};

  // The stack is mmap allocated
  REQUIRE(machine.memory.stack_initial() > machine.memory.mmap_start());
  // The starting address is somewhere in the program area
  REQUIRE(machine.memory.start_address() > 0x10000);
  REQUIRE(machine.memory.start_address() < machine.memory.heap_address());
  // The start address is within the current executable area
  REQUIRE(!machine.cpu.current_execute_segment().empty());
  REQUIRE(machine.cpu.current_execute_segment().is_within(machine.memory.start_address()));
}

TEST_CASE("Execute minimal machine", "[Minimal]") {
  const auto binary = load("://freestanding/basic_b.elf");
  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  machine.install_syscall_handler(1, [](auto &machine) { machine.stop(); });
  machine.simulate(10);
  REQUIRE(machine.return_value<int>() == 666);

#ifdef RISCV_SPAN_AVAILABLE
  // Test span-based machine instantiation
  riscv::Machine<uint64_t> machine_span{std::span<const uint8_t>(binary), {.memory_max = MAX_MEMORY}};
  machine_span.install_syscall_handler(1, [](auto &machine) { machine.stop(); });
  machine_span.simulate(10);
  REQUIRE(machine_span.return_value<int>() == 666);
#endif // RISCV_SPAN_AVAILABLE
}

TEST_CASE("Execution timeout", "[Minimal]") {
  const auto binary = load("://freestanding/basic_c.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // Simulate 250k instructions before giving up
  REQUIRE_THROWS_WITH([&] { machine.simulate(250'000); }(), Catch::Matchers::ContainsSubstring("limit reached"));
}

TEST_CASE("Catch output from write system call", "[Output]") {
  struct State {
    bool output_is_hello_world = false;
  } state;
  const auto binary = load("://freestanding/basic_scall.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls(false, false);
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"basic"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.set_userdata(&state);
  machine.set_printer([](const auto &m, const char *data, size_t size) {
    auto *state = m.template get_userdata<State>();
    std::string text{data, data + size};
    state->output_is_hello_world = (text == "Hello World!");
  });
  // Run for at most X instructions before giving up
  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value<int>() == 666);

  // We require that the write system call forwarded to the printer
  // and the data matched 'Hello World!'.
  REQUIRE(state.output_is_hello_world);
}

TEST_CASE("Calculate fib(50)", "[Compute]") {
  const auto binary = load("://freestanding/basic_fib.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls(false, false);
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"basic", "50"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
  // Run for at most X instructions before giving up
  char b[1024];
  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value<long>() == -298632863);
}

TEST_CASE("Count using EBREAK", "[Compute]") {
  const auto binary = load("://freestanding/basic_ebreak.elf");

  riscv::Machine<uint64_t> machine{binary};
  machine.setup_linux_syscalls(false, false);
  machine.setup_linux({"basic", "50"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  // Count the number of times EBREAK occurs
  static struct {
    unsigned value = 0;
  } counter;

  machine.install_syscall_handler(RISCV_SYSCALL_EBREAK_NR, [](auto &machine) {
    counter.value += 1;
    // EBREAK cannot (currently) stop because it is not
    // treated like a regular system call. Although we
    // can still throw an exception in order to end.
    if (counter.value == 25) machine.stop();
  });
  char b[1024];
  machine.simulate(MAX_INSTRUCTIONS);

  // Tail-call can exit immediately, and will return 25 (which is fine)
  REQUIRE((counter.value == 51 || counter.value == 25));
  if (counter.value == 51) REQUIRE(machine.return_value<long>() == -298632863);
  else REQUIRE(machine.return_value<long>() == 46368L);
}

TEST_CASE("Verify CRC32") {
  const auto binary = load("://freestanding/checksum.elf");
  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"checksum"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
  // Run for at most X instructions before giving up
  // riscv::DebugMachine<uint64_t> dbg{machine};
  // dbg.verbose_instructions = true;
  // dbg.simulate(MAX_INSTRUCTIONS);
  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value() == 0);
}

TEST_CASE("VM function call", "[VMCall]") {
  struct State {
    bool output_is_hello_world = false;
  } state;
  const auto binary = load("://freestanding/vmcall.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.set_userdata(&state);
  machine.set_printer([](const auto &m, const char *data, size_t size) {
    auto *state = m.template get_userdata<State>();
    std::string text{data, data + size};
    state->output_is_hello_world = (text == "Hello World!");
  });
  // Run for at most X instructions before giving up
  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value<int>() == 666);
  REQUIRE(!state.output_is_hello_world);

  const auto hello_address = machine.address_of("hello");
  REQUIRE(hello_address != 0x0);

  // Execute guest function
  machine.vmcall(hello_address);

  // Now hello world should have been printed
  REQUIRE(state.output_is_hello_world);
}

TEST_CASE("VM call return values", "[VMCall]") {
  const auto binary = load("://freestanding/vmcall_return.elf");

  // Linker script emits text to same segment as data, and makes it RWX instead of RX.
  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY, .allow_write_exec_segment = true}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  const auto hello_address = machine.address_of("hello");
  REQUIRE(hello_address != 0x0);

  // Test returning a string
  machine.vmcall(hello_address);
  REQUIRE(machine.return_value<std::string>() == "Hello World!");

  const auto structs_address = machine.address_of("structs");
  REQUIRE(structs_address != 0x0);

  // Test returning a structure
  struct Data {
    int val1;
    int val2;
    float f1;
  };
  machine.vmcall(structs_address);

  const auto data = machine.return_value<Data>();
  REQUIRE(data.val1 == 1);
  REQUIRE(data.val2 == 2);
  REQUIRE(data.f1 == 3.0f);

  // Test returning a pointer to a structure
  const auto *data_ptr = machine.return_value<Data *>();
  REQUIRE(data_ptr->val1 == 1);
  REQUIRE(data_ptr->val2 == 2);
  REQUIRE(data_ptr->f1 == 3.0f);
}

TEST_CASE("VM call enum values", "[VMCall]") {
  const auto binary = load("://freestanding/vmcall_enum.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  // Test Enum values
  enum class MyEnum : int {
    Hello = 1,
    World = 2,
  };

  machine.install_syscall_handler(0, [](auto &machine) {
    auto [value] = machine.template sysargs<MyEnum>();
    REQUIRE(value == MyEnum::Hello);
    machine.set_result(MyEnum::World);
  });

  machine.vmcall("mycall", MyEnum::Hello);
  REQUIRE(machine.return_value<MyEnum>() == MyEnum::World);
}

TEST_CASE("VM call and STOP instruction", "[VMCall]") {
  struct State {
    bool output_is_hello_world = false;
  } state;
  const auto binary = load("://freestanding/vmcall_stop.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.set_userdata(&state);
  machine.set_printer([](const auto &m, const char *data, size_t size) {
    auto *state = m.template get_userdata<State>();
    std::string text{data, data + size};
    state->output_is_hello_world = (text == "Hello World!");
  });

  machine.install_syscall_handler(500, [](auto &machine) {
    auto [arg0] = machine.template sysargs<int>();
    REQUIRE(arg0 == 1234567);

    const auto func = machine.address_of("preempt");
    REQUIRE(func != 0x0);

    auto result = machine.preempt(15'000ull, func, strlen("Hello World!"));
    REQUIRE(result == 777);
  });

  REQUIRE(!state.output_is_hello_world);

  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(state.output_is_hello_world);
  REQUIRE(machine.return_value<int>() == 777);

  for (int i = 0; i < 10; i++) {
    state.output_is_hello_world = false;

    const auto func = machine.address_of("start");
    REQUIRE(func != 0x0);

    // Execute guest function
    machine.vmcall<15'000ull>(func);
    REQUIRE(machine.return_value<int>() == 1234);

    // Now hello world should have been printed
    REQUIRE(state.output_is_hello_world);
  }
}

TEST_CASE("VM call with arrays and vectors", "[VMCall]") {
  const auto binary = load("://freestanding/vmcall_array.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.simulate(MAX_INSTRUCTIONS);
  REQUIRE(machine.return_value<int>() == 666);

  // Test passing an integer array
  std::array<int, 3> iarray = {1, 2, 3};
  // The array is pushed on the stack, so it becomes a sequential pointer argument
  int res1 = machine.vmcall("pass_iarray", iarray, iarray.size());
  REQUIRE(res1 == 1);

  // A const-reference to an array should also work
  const std::array<int, 3> &array_ref = iarray;
  int res2 = machine.vmcall("pass_iarray", array_ref, array_ref.size());
  REQUIRE(res2 == 1);

  // Test passing a float array
  std::array<float, 3> farray = {1.0f, 2.0f, 3.0f};
  int res3 = machine.vmcall("pass_farray", farray, farray.size());
  REQUIRE(res3 == 1);

  // Test passing a vector
  struct Data {
    int val1;
    int val2;
    float f1;
  };
  std::vector<Data> vec = {
      {1, 2, 3.0f},
      {4, 5, 6.0f},
      {7, 8, 9.0f},
  };
  // The vector is pushed on the stack, so it becomes a sequential pointer argument
  int res4 = machine.vmcall("pass_struct", vec, vec.size());
  REQUIRE(res4 == 1);

  // A const-reference to a vector should also work
  const std::vector<Data> &vec_ref = vec;
  int res5 = machine.vmcall("pass_struct", vec_ref, vec_ref.size());
  REQUIRE(res5 == 1);
}

TEST_CASE("VM call and preemption", "[VMCall]") {
  struct State {
    bool output_is_hello_world = false;
  } state;
  const auto binary = load("://freestanding/vmcall_preempt.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.set_userdata(&state);
  machine.set_printer([](const auto &m, const char *data, size_t size) {
    auto *state = m.template get_userdata<State>();
    std::string text{data, data + size};
    state->output_is_hello_world = (text == "Hello World!");
  });

  machine.install_syscall_handler(500, [](auto &machine) {
    auto [arg0] = machine.template sysargs<int>();
    REQUIRE(arg0 == 1234567);

    const auto func = machine.address_of("preempt");
    REQUIRE(func != 0x0);

    machine.preempt(15'000ull, func, strlen("Hello World!"));
  });

  REQUIRE(!state.output_is_hello_world);

  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(state.output_is_hello_world);
  REQUIRE(machine.return_value<int>() == 666);

  for (int i = 0; i < 10; i++) {
    state.output_is_hello_world = false;

    const auto func = machine.address_of("start");
    REQUIRE(func != 0x0);

    // Execute guest function
    machine.vmcall<15'000ull>(func);
    REQUIRE(machine.return_value<int>() == 1);

    // Now hello world should have been printed
    REQUIRE(state.output_is_hello_world);
  }
}

TEST_CASE("VM function call in fork", "[VMCall]") {
  // The global variable 'value' should get
  // forked as value=1. We assert this, then
  // we set value=0. New forks should continue
  // to see value=1 as they are forked from the
  // main VM where value is still 0.
  const auto binary = load("://freestanding/vmcall_fork.elf");

  riscv::Machine<uint64_t> machine{binary,
                                   {
                                       .memory_max = MAX_MEMORY,
                                       .allow_write_exec_segment = true,
                                       .use_memory_arena = false,
                                   }};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.simulate(MAX_INSTRUCTIONS);
  REQUIRE(machine.return_value<int>() == 666);

  // Test many forks
  for (size_t i = 0; i < 10; i++) {
    riscv::Machine<uint64_t> fork{machine, {}};
    REQUIRE(fork.memory.uses_flat_memory_arena() == false);

    fork.set_printer([](const auto &, const char *data, size_t size) {
      std::string text{data, data + size};
      REQUIRE(text == "Hello World!");
    });

    const auto hello_address = fork.address_of("hello");
    REQUIRE(hello_address != 0x0);

    // Execute guest function
    fork.vmcall(hello_address);

    int res1 = fork.vmcall("str", "Hello");
    REQUIRE(res1 == 1);

    res1 = fork.vmcall("str", std::string("Hello"));
    REQUIRE(res1 == 1);

    std::string hello = "Hello";
    const std::string &ref = hello;

    res1 = fork.vmcall("str", ref);
    REQUIRE(res1 == 1);

    struct {
      int v1 = 1;
      int v2 = 2;
      float f1 = 3.0f;
    } data;
    int res2 = fork.vmcall("structs", data);
    REQUIRE(res2 == 2);

    long intval = 456;
    long &intref = intval;

    int res3 = fork.vmcall("ints", 123L, intref, (long &&)intref);
    REQUIRE(res3 == 3);

    int res4 = fork.vmcall("fps", 1.0f, 2.0);
    REQUIRE(res4 == 4);

    // XXX: Binary translation currently "remembers" that arena
    // was enabled, and will not disable it for the fork.
    if constexpr (riscv::flat_readwrite_arena && riscv::binary_translation_enabled) return;
  }
}

TEST_CASE("Verify floating point instructions", "[Verification]") {
  const auto binary = load("://freestanding/fp_verify.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"compute_pi"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
  // Run for at most X instructions before giving up
  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value() == 0);
}

TEST_CASE("Compute PI slowly", "[Verification]") {
  const auto binary = load("://freestanding/fp_pi.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY, .allow_write_exec_segment = true}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"compute_pi"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
  // Run for at most X instructions before giving up
  machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value() == 0);
}

TEST_CASE("Writes to read-only segment", "[Memory][!throws]") {
  static const uint64_t MAX_MEMORY = 8ul << 20; // 8MB
  static const uint64_t MAX_INSTRUCTIONS = 10'000'000ul;

  const auto binary = load("://freestanding/memprotect.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"rodata"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  REQUIRE_THROWS_WITH([&] { machine.memory.write<uint8_t>(machine.cpu.pc(), 0); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));

  // Guard pages are not writable
  REQUIRE_THROWS_WITH([&] { machine.simulate(MAX_INSTRUCTIONS); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));

  REQUIRE(machine.return_value<int>() != 666);

  const auto write_addr = machine.address_of("write_to");
  REQUIRE(write_addr != 0x0);
  const auto read_addr = machine.address_of("read_from");
  REQUIRE(read_addr != 0x0);

  // Reads amd writes to invalid locations
  REQUIRE_THROWS_WITH([&] { machine.vmcall<MAX_INSTRUCTIONS>(read_addr, 0); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));

  machine.vmcall<MAX_INSTRUCTIONS>(read_addr, 0x1000);

  for (uint64_t addr = machine.memory.start_address(); addr < machine.memory.initial_rodata_end(); addr += 0x1000) {
    REQUIRE_THROWS_WITH([&] { machine.vmcall<MAX_INSTRUCTIONS>(write_addr, addr); }(),
                        Catch::Matchers::ContainsSubstring("Protection fault"));
  }
}

/**
 * These tests are designed to be really brutal to support,
 * and most emulators will surely fail here.
 */
TEST_CASE("Calculate fib(50) slowly, basic", "[Compute]") {
  const auto binary = load("://freestanding/brutal_fib50.elf");

  riscv::Machine<uint64_t> machine{binary, {.use_memory_arena = false}};

  const auto addr = machine.address_of("my_start");
  REQUIRE(addr != 0x0);

  // Create a manual VM call in order to avoid exercising the C-runtime
  // The goal is to see if the basic start/stop/resume functionality works
  machine.cpu.jump(addr);
  machine.cpu.reg(riscv::REG_ARG0) = 50;
  machine.cpu.reg(riscv::REG_RA) = machine.memory.exit_address();

  riscv::Machine<uint64_t> fork{machine};
  do {
    // No matter how many (or few) instructions we execute before exiting
    // simulation, we should be able to resume and complete the program normally.
    for (int step = 5; step < 105; step++) {
      fork.cpu.registers() = machine.cpu.registers();
      do {
        fork.simulate<false>(step);
      } while (fork.instruction_limit_reached());
      REQUIRE(fork.return_value<int64_t>() == int64_t(12586269025L));
    }
    machine.simulate<false>(100);
  } while (machine.instruction_limit_reached());
}

TEST_CASE("Execute libc_start_main, slowly", "[Compute]") {
  const auto binary = load("://freestanding/basic_a.elf");

  riscv::Machine<uint64_t> machine{binary, {.use_memory_arena = false}};
  machine.setup_linux_syscalls();
  machine.setup_linux({"brutal", "50"}, {"LC_TYPE=C", "LC_ALL=C"});

  do {
    // No matter how many (or few) instructions we execute before exiting
    // simulation, we should be able to resume and complete the program normally.
    for (int step = 5; step < 105; step++) {
      riscv::Machine<uint64_t> fork{machine, {.use_memory_arena = false}};
      do {
        fork.simulate<false>(step);
      } while (fork.instruction_limit_reached());
      REQUIRE(fork.return_value<long>() == 666);
    }
    machine.simulate<false>(1000);
  } while (machine.instruction_limit_reached());

  REQUIRE(machine.return_value<long>() == 666);
}

struct InstructionState {
  std::array<std::any, 8> args;
};

/** The new custom instruction **/
static const riscv::Instruction<uint64_t> custom_instruction_handler{
    [](riscv::CPU<uint64_t> &cpu, riscv::rv32i_instruction instr) {
      printf("Hello custom instruction World!\n");
      REQUIRE(instr.opcode() == 0b1011011);

      auto *state = cpu.machine().get_userdata<InstructionState>();
      // Argument number
      const unsigned idx = instr.Itype.rd & 7;
      // Select type and retrieve value from argument registers
      switch (instr.Itype.funct3) {
      case 0x0: // Register value (64-bit unsigned)
        state->args[idx] = cpu.reg(riscv::REG_ARG0 + idx);
        break;
      case 0x1: // 64-bit floating point
        state->args[idx] = cpu.registers().getfl(riscv::REG_FA0 + idx).f64;
        break;
      default: throw "Implement me";
      }
    },
    [](char *buffer, size_t len, auto &, riscv::rv32i_instruction instr) {
      return snprintf(buffer, len, "CUSTOM: 4-byte 0x%X (0x%X)", instr.opcode(), instr.whole);
    }};

TEST_CASE("Custom instruction", "[Custom]") {
  // Build a program that uses a custom instruction to
  // select and identify a system call argument.
  const auto binary = load("://freestanding/custom_instr.elf");

  // Install system call number 500 (used by our program above).
  static bool syscall_was_called = false;

  InstructionState state;

  // Normal (fastest) simulation
  {
    riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
    // Install the handler for unimplemented instructions, allowing us to
    // select our custom instruction for a reserved opcode.
    machine.cpu.on_unimplemented_instruction =
        [](riscv::rv32i_instruction instr) -> const riscv::Instruction<uint64_t> & {
      if (instr.opcode() == 0b1011011) {
        return custom_instruction_handler;
      }
      return riscv::CPU<uint64_t>::get_unimplemented_instruction();
    };
    machine.set_userdata(&state);
    // We need to install Linux system calls for maximum gucciness
    machine.setup_linux_syscalls();
    // We need to create a Linux environment for runtimes to work well
    machine.setup_linux({"custom_instruction"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
    // Run for at most X instructions before giving up
    syscall_was_called = false;
    machine.install_syscall_handler(500, [](riscv::Machine<uint64_t> &machine) {
      auto *state = machine.get_userdata<InstructionState>();
      REQUIRE(std::any_cast<double>(state->args[1]) == 1234.0);
      REQUIRE(std::any_cast<uint64_t>(state->args[3]) == 0xDEADB33F);
      syscall_was_called = true;
    });
    machine.simulate(MAX_INSTRUCTIONS);
    REQUIRE(syscall_was_called == true);
  }
  // Precise (step-by-step) simulation
  {
    riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
    machine.cpu.on_unimplemented_instruction =
        [](riscv::rv32i_instruction instr) -> const riscv::Instruction<uint64_t> & {
      if (instr.opcode() == 0b1011011) {
        return custom_instruction_handler;
      }
      return riscv::CPU<uint64_t>::get_unimplemented_instruction();
    };
    machine.set_userdata(&state);
    machine.setup_linux_syscalls();
    machine.setup_linux({"custom_instruction"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
    machine.install_syscall_handler(500, [](riscv::Machine<uint64_t> &machine) {
      auto *state = machine.get_userdata<InstructionState>();
      REQUIRE(std::any_cast<double>(state->args[1]) == 1234.0);
      REQUIRE(std::any_cast<uint64_t>(state->args[3]) == 0xDEADB33F);
      syscall_was_called = true;
    });
    // Verify step-by-step simulation
    syscall_was_called = false;
    machine.set_max_instructions(MAX_INSTRUCTIONS);
    machine.cpu.simulate_precise();
    REQUIRE(syscall_was_called == true);
  }
}

TEST_CASE("Read and write traps", "[Memory Traps]") {
  struct State {
    bool output_is_hello_world = false;
  } state;
  const auto binary = load("://freestanding/memtrap_rw.elf");

  riscv::Machine<uint64_t> machine{binary, {.allow_write_exec_segment = true}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  constexpr uint64_t TRAP_PAGE = 0xF0000000;
  bool trapped_write = false;
  bool trapped_read = false;

  static constexpr uint32_t mmio_offset = 0x10;
  long mmio_value = 0;

  auto &trap_page = machine.memory.create_writable_pageno(riscv::Memory<uint64_t>::page_number(TRAP_PAGE));
  trap_page.set_trap([&](auto &page, uint32_t offset, int mode, int64_t value) {
    const size_t size = riscv::Page::trap_size(mode);

    // Goal: Store a value written to a special offset.
    // Then read back the stored value when read.
    switch (riscv::Page::trap_mode(mode)) {
    case riscv::TRAP_WRITE:
      if (offset == mmio_offset) {
        REQUIRE(value == 1234);
        REQUIRE(size == 8);
        trapped_write = true;
        // Store the value without writing it to the page.
        mmio_value = value;
      }
      // A trapped page cannot automatically be written to
      // We have to do the write ourselves. Eg.
      // if (size == 8)
      //	page.page().template aligned_write<uint64_t>(offset, value);
      // else if (size == 4)
      //	page.page().template aligned_write<uint32_t>(offset, value);
      break;
    case riscv::TRAP_READ:
      if (offset == mmio_offset) {
        REQUIRE(value == 0);
        trapped_read = true;
        // We cannot return the read value here, but we can modify the
        // page data here, and instead we can control the value indirectly.
        // This lets us decide what to return dynamically.
        // Note how we write the 'mmio_value'.
        page.page().template aligned_write<uint64_t>(offset, mmio_value);
      }
      break;
    }
  });

  // Run for at most X instructions before giving up
  riscv::DebugMachine<uint64_t> dbg{machine};
  dbg.verbose_instructions = true;
  dbg.simulate();
  // machine.simulate(MAX_INSTRUCTIONS);

  REQUIRE(machine.return_value<int>() == 666);
  REQUIRE(trapped_read == false);
  REQUIRE(trapped_write == false);

  // Write 1234 to the trapped page, should cause TRAP_WRITE
  auto call_addr = machine.memory.resolve_address("hello_write");
  // reset the stack pointer to an initial location (deliberately)
  machine.cpu.reset_stack_pointer();
  // setup calling convention
  machine.setup_call(1234);
  machine.cpu.jump(call_addr);
  dbg.simulate();
  REQUIRE(trapped_write == true);
  REQUIRE(trapped_read == false);
  trapped_write = false;

  // Read from the trapped page, should cause TRAP_READ
  machine.vmcall("hello_read");
  REQUIRE(trapped_write == false);
  REQUIRE(trapped_read == true);
  REQUIRE(machine.return_value() == 1234);
}

TEST_CASE("Execute traps", "[Memory Traps]") {
  const auto binary = load("://freestanding/memtrap_exec.elf");

  riscv::Machine<uint64_t> machine{binary};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  constexpr uint64_t TRAP_PAGE = 0xF0000000;

  // Install exit(666) code at TRAP_PAGE
  static const std::array<uint32_t, 3> dont_execute_this{
      0x29a00513, //        li      a0,666
      0x05d00893, //        li      a7,93
      0x00000073, //        ecall
  };
  machine.copy_to_guest(TRAP_PAGE, dont_execute_this.data(), 12);

  auto &trap_page = machine.memory.create_writable_pageno(riscv::Memory<uint64_t>::page_number(TRAP_PAGE));
  trap_page.attr.exec = true;
  trap_page.attr.read = false;
  trap_page.attr.write = false;

  bool trapped_exec = false;

  trap_page.set_trap([&](auto &, uint32_t offset, int mode, int64_t value) {
    switch (riscv::Page::trap_mode(mode)) {
    case riscv::TRAP_EXEC:
      REQUIRE(offset == 0x0);
      REQUIRE(value == TRAP_PAGE);
      trapped_exec = true;
      // Return to caller
      machine.cpu.jump(machine.cpu.reg(riscv::REG_RA));
      break;
    default: throw std::runtime_error("Nope");
    }
  });

  // Using _exit we can run this test in a loop
  const auto main_addr = machine.address_of("main");

  for (size_t i = 0; i < 15; i++) {
    machine.cpu.jump(main_addr);
    trapped_exec = false;
    machine.simulate(MAX_INSTRUCTIONS);

    REQUIRE(machine.return_value<int>() == 1234);
    REQUIRE(trapped_exec == true);
  }
}

TEST_CASE("Override execute space protection fault", "[Memory Traps]") {
  struct State {
    bool trapped_fault = false;
    const uint64_t TRAP_ADDR = 0xF0000000;
  } state;
  const auto binary = load("://freestanding/memtrap_exec.elf");

  riscv::Machine<uint64_t> machine{binary};
  machine.setup_linux_syscalls();
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  machine.set_userdata(&state);
  machine.cpu.set_fault_handler([](auto &cpu, auto &) {
    auto &state = *cpu.machine().template get_userdata<State>();
    // We can successfully handle an execute space protection
    // fault by returning back to caller.
    if (cpu.pc() == state.TRAP_ADDR) {
      state.trapped_fault = true;
      // Return to caller
      cpu.jump(cpu.reg(riscv::REG_RA));
      return;
    }
    // CPU is not where we wanted
    cpu.trigger_exception(riscv::EXECUTION_SPACE_PROTECTION_FAULT, cpu.pc());
  });

  // Install exit(666) code at TRAP_PAGE
  static const std::array<uint32_t, 3> dont_execute_this{
      0x29a00513, //        li      a0,666
      0x05d00893, //        li      a7,93
      0x00000073, //        ecall
  };
  machine.copy_to_guest(state.TRAP_ADDR, dont_execute_this.data(), 12);

  // Make sure the page is not executable
  auto &trap_page = machine.memory.create_writable_pageno(riscv::Memory<uint64_t>::page_number(state.TRAP_ADDR));
  trap_page.attr.exec = false;

  // Using _exit we can run this test in a loop
  const auto main_addr = machine.address_of("main");

  for (size_t i = 0; i < 15; i++) {
    machine.cpu.jump(main_addr);
    state.trapped_fault = false;
    machine.simulate(MAX_INSTRUCTIONS);

    REQUIRE(machine.return_value<int>() == 1234);
    REQUIRE(state.trapped_fault == true);
  }
}

TEST_CASE("Calculate fib(2560000) on execute page", "[VA]") {
  const auto binary = load("://freestanding/va_execute.elf");

  static constexpr uint32_t VA_FUNC = 0xF0000000;

  // Normal (fastest) simulation
  {
    riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
    // We need to install Linux system calls for maximum gucciness
    machine.setup_linux_syscalls();
    // We need to create a Linux environment for runtimes to work well
    machine.setup_linux({"va_exec"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
    // Run for at most X instructions before giving up
    machine.simulate(MAX_INSTRUCTIONS);

    REQUIRE(machine.return_value<long>() == 12586269025L);

    // VM call into new execute segment
    REQUIRE(machine.vmcall(VA_FUNC, 50, 0, 1) == 12586269025L);
  }
  // Precise (step-by-step) simulation
  {
    riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
    machine.setup_linux_syscalls();
    machine.setup_linux({"va_exec"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
    // Verify step-by-step simulation
    machine.set_max_instructions(MAX_INSTRUCTIONS);
    machine.cpu.simulate_precise();

    REQUIRE(machine.return_value<long>() == 12586269025L);

    // VM call into new execute segment
    REQUIRE(machine.vmcall(VA_FUNC, 50, 0, 1) == 12586269025L);
  }
  // Debug-assisted simulation
  {
    riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
    machine.setup_linux_syscalls();
    machine.setup_linux({"va_exec"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

    riscv::DebugMachine debugger{machine};
    // debugger.verbose_instructions = true;

    // Verify step-by-step simulation
    debugger.simulate(MAX_INSTRUCTIONS);

    REQUIRE(machine.return_value<long>() == 12586269025L);

    // VM call into new execute segment
    REQUIRE(machine.vmcall(VA_FUNC, 50, 0, 1) == 12586269025L);
  }
}

TEST_CASE("Sequential buffer", "[Buffer]") {
  const auto binary = load("://freestanding/basic_a.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  auto origin = machine.memory.mmap_allocate(1);

  static const char hello[] = "hello world!";

  for (auto addr = origin; addr < origin + 4096 - 12; addr++) {
    machine.memory.memcpy(addr, hello, sizeof(hello));

    auto buf = machine.memory.membuffer(addr, 12);
    REQUIRE(buf.is_sequential());
    REQUIRE(buf.size() == 12);
    REQUIRE(buf.strview() == "hello world!");
    REQUIRE(buf.to_string() == "hello world!");
  }

  // maxlen works
  REQUIRE_THROWS_WITH([&] { machine.memory.membuffer(origin, 128, 127); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));
}

TEST_CASE("Boundary buffer", "[Buffer]") {
  const auto binary = load("://freestanding/basic_a.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls();
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"vmcall"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});

  auto origin = machine.memory.mmap_allocate(1);

  static const char hello[] = "hello world!";
  char buffer[13];

  for (auto addr = origin + 4096 - 11; addr < origin + 4095; addr++) {
    machine.memory.memcpy(addr, hello, sizeof(hello));
    auto buf = machine.memory.membuffer(addr, 12);
    REQUIRE(buf.is_sequential() == false);
    REQUIRE(buf.size() == 12);
    REQUIRE(buf.to_string() == "hello world!");
    // String view is no longer possible
    buf.copy_to(buffer, 12);
    buffer[12] = 0;
    REQUIRE(std::string(buffer) == "hello world!");
  }
}

int main(int argc, char *argv[]) {
  QCoreApplication ap(argc, argv);
  return Catch::Session().run(argc, argv);
}
