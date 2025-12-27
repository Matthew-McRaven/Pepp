#include <QCoreApplication>
#include <QDirIterator>
#include <catch.hpp>
#include "loader.hpp"
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

static const uint64_t MAX_MEMORY = 8ul << 20; /* 8MB */
static const uint64_t MAX_INSTRUCTIONS = 10'000'000ul;

TEST_CASE("Instantiate machine", "[Instantiate]") {
  const auto binary = load("://hosted/basic_a.elf");

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
  return;
  struct State {
    bool output_is_hello_world = false;
  } state;
  const auto binary = load("://hosted/basic_scall_write.elf");

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
/*
TEST_CASE("Calculate fib(50)", "[Compute]") {
  const auto binary = load("://riscv_samples/basic_fib.elf");

  riscv::Machine<uint64_t> machine{binary, {.memory_max = MAX_MEMORY}};
  // We need to install Linux system calls for maximum gucciness
  machine.setup_linux_syscalls(false, false);
  // We need to create a Linux environment for runtimes to work well
  machine.setup_linux({"basic", "50"}, {"LC_TYPE=C", "LC_ALL=C", "USER=root"});
  // Run for at most X instructions before giving up
  char b[1024];
  while (true) {
    auto ni = machine.cpu.read_next_instruction();
    auto di = machine.cpu.decode(ni);
    auto count = di.printer(b, sizeof(b), machine.cpu, ni);
    qDebug().noquote().nospace() << QString::fromLocal8Bit((const char *)b, count);
    machine.cpu.step_one(true);
  }

  REQUIRE(machine.return_value<long>() == -298632863);
}

TEST_CASE("Count using EBREAK", "[Compute]") {
  const auto binary = load("://riscv_samples/basic_ebreak.elf");

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
  while (true) {
    auto ni = machine.cpu.read_next_instruction();
    auto di = machine.cpu.decode(ni);
    auto count = di.printer(b, sizeof(b), machine.cpu, ni);
    qDebug().noquote().nospace() << QString::fromLocal8Bit((const char *)b, count);
    machine.cpu.step_one(true);
  }

  // Tail-call can exit immediately, and will return 25 (which is fine)
  REQUIRE((counter.value == 51 || counter.value == 25));
  if (counter.value == 51) REQUIRE(machine.return_value<long>() == -298632863);
  else REQUIRE(machine.return_value<long>() == 46368L);
}*/

int main(int argc, char *argv[]) {
  QCoreApplication ap(argc, argv);
  return Catch::Session().run(argc, argv);
}
