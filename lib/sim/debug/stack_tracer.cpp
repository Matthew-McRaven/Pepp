#include "stack_tracer.hpp"
#include <spdlog/spdlog.h>

pepp::debug::StackTracer::StackTracer() : _logger(spdlog::get("debugger::stack")) {}

void pepp::debug::StackTracer::notifyInstruction(quint16 pc, InstructionType type) {
  switch (type) {
  case InstructionType::CALL: _logger->info("CALL  at {:#04x}", pc); break;
  case InstructionType::RET: _logger->info("RET   at {:#04x}", pc); break;
  case InstructionType::TRAP: _logger->info("SCALL at {:#04x}", pc); break;
  case InstructionType::TRAPRET: _logger->info("SRET at {:#04x}", pc); break;
  case InstructionType::ADDITIVE: _logger->info("ADD   at {:#04x}", pc); break;
  case InstructionType::ASSIGNMENT: _logger->info("SET   at {:#04x}", pc); break;
  }
}
