#include "core/sim/debugger/tvm_interpreter.hpp"

namespace tvm {

Interpreter::Interpreter(std::shared_ptr<pepp::bts::BufferManager> mgr, std::unique_ptr<Backend> backend)
    : _mgr(std::move(mgr)), _decoder(_mgr, _state), _backend(std::move(backend)) {}


void Interpreter::step() {
  _decoder.decode();
  // Decode can fail (bad instruction buffer, out-of-range DP for a sync op, unknown opcode), in which case decoded()
  // still holds the previous instruction. Re-check L before handing anything to the backend.
  if (_state.csrs.L) _backend->dispatch(_state, _decoder.decoded());
}

void Interpreter::run(pepp::bts::Buffer::Location loc, RegisterRetention retain) {
  _state.restart(retain);
  _state.update_ip(loc);
  while (_state.csrs.L) step();
}

void Interpreter::run(ProgramLocation loc, RegisterRetention retain) {
  _state.restart(retain);
  // Seed DP before IP so the body starts with its payload already addressed. Skipped for a record that carries no
  // payload. Clobbering DP would break a program that steps relative to the one before it.
  if (loc.data.id != pepp::bts::Buffer::ID{0}) _state.update_dp(loc.data);
  _state.update_ip(loc.code);
  while (_state.csrs.L) step();
}

std::size_t Interpreter::run_each(std::span<const pepp::bts::Buffer::Location> locs, RegisterRetention retain) {
  // Run the program at each location. Check for a hard stop condition. On hard stop, abort the loop.
  // On a normal/soft stop, resume execution of the next program.
  std::size_t count = 0;
  for (const auto &loc : locs) {
    run(loc, retain);
    ++count;
    if (_state.csrs.F == 1) break;
  }
  return count;
}

std::size_t Interpreter::run_each(std::span<const ProgramLocation> locs, RegisterRetention retain) {
  std::size_t count = 0;
  for (const auto &loc : locs) {
    run(loc, retain);
    ++count;
    if (_state.csrs.F == 1) break;
  }
  return count;
}

} // namespace tvm
