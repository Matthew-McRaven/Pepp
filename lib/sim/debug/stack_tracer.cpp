#include "stack_tracer.hpp"
#include <spdlog/spdlog.h>

pepp::debug::StackTracer::StackTracer() : _logger(spdlog::get("debugger::stack")) {}

void pepp::debug::StackTracer::notifyInstruction(quint16 pc, quint16 spAfter, InstructionType type) {
  std::string cmds_str = "";
  if (auto cmds = _debug_info.commands.find(pc); cmds != _debug_info.commands.end()) {
    cmds_str = QString(cmds.value()).toStdString();
  }
  std::string sp_str = "";
  if (_lastSP) {
    qint16 diff = qint16(spAfter) - qint16(*_lastSP);
    sp_str = fmt::format("SP:{:04x}/{:<+5}", spAfter, diff);
  } else {
    sp_str = fmt::format("SP:{:04x}     ", spAfter);
  }
  _lastSP = spAfter;

  switch (type) {
  case InstructionType::CALL: _logger->info("CALL    at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  case InstructionType::RET: _logger->info("RET     at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  case InstructionType::TRAP: _logger->info("SCALL   at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  case InstructionType::TRAPRET: _logger->info("SRET    at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  case InstructionType::ALLOCATE: _logger->info("ALLOC   at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  case InstructionType::DEALLOCATE: _logger->info("DEALLOC at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  case InstructionType::ASSIGNMENT: _logger->info("SET     at PC:{:04x} {}  {}", pc, sp_str, cmds_str); break;
  }
}
