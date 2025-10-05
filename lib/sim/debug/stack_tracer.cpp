#include "stack_tracer.hpp"
#include <spdlog/spdlog.h>

pepp::debug::StackTracer::StackTracer() : _logger(spdlog::get("debugger::stack")) {}

void pepp::debug::StackTracer::notifyInstruction(quint16 pc, InstructionType type) {
  std::string cmds_str = "";
  if (auto cmds = _debug_info.commands.find(pc); cmds != _debug_info.commands.end()) {
    cmds_str = QString(cmds.value()).toStdString();
  }
  switch (type) {
  case InstructionType::CALL: _logger->info("CALL    at 0x{:04x}  {}", pc, cmds_str); break;
  case InstructionType::RET: _logger->info("RET     at 0x{:04x}  {}", pc, cmds_str); break;
  case InstructionType::TRAP: _logger->info("SCALL   at 0x{:04x}  {}", pc, cmds_str); break;
  case InstructionType::TRAPRET: _logger->info("SRET    at 0x{:04x}  {}", pc, cmds_str); break;
  case InstructionType::ALLOCATE: _logger->info("ALLOC   at 0x{:04x}  {}", pc, cmds_str); break;
  case InstructionType::DEALLOCATE: _logger->info("DEALLOC at 0x{:04x}  {}", pc, cmds_str); break;
  case InstructionType::ASSIGNMENT: _logger->info("SET     at 0x{:04x}  {}", pc, cmds_str); break;
  }
}
