#include <QCoreApplication>
#include <QDirIterator>
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

int main(int argc, char *argv[]) {
  QCoreApplication ap(argc, argv);
  return Catch::Session().run(argc, argv);
}
