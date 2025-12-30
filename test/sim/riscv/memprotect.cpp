#include <catch.hpp>
#include "sim3/systems/notraced_riscv_isa3_system.hpp"

static const std::vector<uint8_t> empty;
static constexpr uint32_t V = 0x1000;
static constexpr uint32_t VLEN = 16 * riscv::Page::size();

TEST_CASE("RISC-V basic page protections", "[scope:sim][kind:int][arch:RV][!throws]") {
  riscv::Machine<uint32_t> machine{empty, {.use_memory_arena = false}};

  machine.memory.set_page_attr(V, VLEN, {.read = false, .write = true, .exec = false});
  machine.memory.memset(V, 0, VLEN);
  machine.memory.set_page_attr(V, VLEN, {.read = false, .write = false, .exec = true});

  machine.cpu.jump(V);
  REQUIRE(machine.cpu.pc() == V);
  // The data at V is all zeroes, which forms an
  // illegal instruction in RISC-V.
  REQUIRE_THROWS_WITH([&] { machine.simulate(1); }(), Catch::Matchers::ContainsSubstring("Illegal opcode executed"));

  // V is not readable anymore
  REQUIRE_THROWS_WITH([&] { machine.memory.membuffer(V, VLEN); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));

  REQUIRE_THROWS_WITH([&] { machine.memory.memview(V, VLEN); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));

  REQUIRE_THROWS_WITH([&] { machine.memory.memstring(V); }(), Catch::Matchers::ContainsSubstring("Protection fault"));

  // V is not writable anymore
  REQUIRE_THROWS_WITH([&] { machine.memory.memset(V, 0, VLEN); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));

  REQUIRE_THROWS_WITH([&] { machine.memory.memcpy(V, "1234", 4); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));
}

TEST_CASE("Trigger guard pages", "[scope:sim][kind:int][arch:RV][!throws]") {
  riscv::Machine<uint32_t> machine{empty};

  machine.memory.install_shared_page(0, riscv::Page::guard_page());
  machine.memory.install_shared_page(17, riscv::Page::guard_page());
  machine.memory.memset(V, 0, VLEN);

  // V is not executable
  auto f = [&] {
    machine.cpu.jump(V);
    machine.simulate(1);
  };
  REQUIRE_THROWS_WITH(f(), Catch::Matchers::ContainsSubstring("Execution space protection fault"));

  // Guard pages are not writable
  REQUIRE_THROWS_WITH([&] { machine.memory.memset(V - 4, 0, 4); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));
  REQUIRE_THROWS_WITH([&] { machine.memory.memset(V + 16 * riscv::Page::size(), 0, 4); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));
}

TEST_CASE("RISC-V misaligned page attributes", "[scope:sim][kind:unit][arch:RV]") {
  riscv::Machine<uint32_t> machine{empty};

  machine.memory.memset(V, 0, VLEN);
  machine.memory.set_page_attr(V + 4095, 2, {.read = false, .write = false, .exec = false});
  REQUIRE(machine.memory.get_page(V + 0).attr.read == false);
  REQUIRE(machine.memory.get_page(V + 4095).attr.read == false);
  REQUIRE(machine.memory.get_page(V + 4096).attr.read == false);
  REQUIRE(machine.memory.get_page(V + 8191).attr.read == false);
  REQUIRE(machine.memory.get_page(V + 8192).attr.read == true);
  machine.memory.set_page_attr(V + 4095, 1, {.read = true, .write = true, .exec = true});
  REQUIRE(machine.memory.get_page(V + 0).attr.read == true);
  REQUIRE(machine.memory.get_page(V + 4095).attr.read == true);
  REQUIRE(machine.memory.get_page(V + 4096).attr.read == false);
  REQUIRE(machine.memory.get_page(V + 8191).attr.read == false);
  REQUIRE(machine.memory.get_page(V + 8192).attr.read == true);
}

TEST_CASE("RISC-V page caches must be invalidated", "[scope:sim][kind:int][arch:RV][!throws]") {
  riscv::Machine<uint32_t> machine{empty};

  // Force creation of writable pages
  machine.memory.memset(V, 0, VLEN);
  // Read data from page, causing cached read
  REQUIRE(machine.memory.read<uint32_t>(V) == 0x0);
  // Make page completely unpresented
  machine.memory.set_page_attr(V, riscv::Page::size(), {.read = false, .write = false, .exec = false});
  // We can still read from the page, because
  // it is in the read cache.
  REQUIRE(machine.memory.read<uint32_t>(V) == 0x0);

  // Invalidate the caches
  machine.memory.invalidate_reset_cache();

  // We can no longer read from the page
  REQUIRE_THROWS_WITH([&] { machine.memory.read<uint32_t>(V); }(),
                      Catch::Matchers::ContainsSubstring("Protection fault"));
}
