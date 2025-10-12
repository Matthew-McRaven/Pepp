#include "stack_tracer.hpp"
#include <spdlog/spdlog.h>
#include "./expr_tokenizer.hpp"
#include "expr_ast.hpp"
#include "expr_parser.hpp"
#include "fmt/ranges.h"

pepp::debug::StackTracer::StackTracer() : _logger(spdlog::get("debugger::stack")) {}

bool pepp::debug::StackTracer::canTrace() const {
  return !_debug_info.commands.empty() && _debug_info.typeInfo != nullptr;
}

const pas::obj::common::DebugInfo &pepp::debug::StackTracer::debugInfo() const { return _debug_info; }

void pepp::debug::StackTracer::setDebugInfo(pas::obj::common::DebugInfo debug_info, Environment *env) {
  _debug_info = debug_info;
  _env = env;
  _lastSP = std::nullopt;
  _activeStack = nullptr;
  _stacks.clear();
  using namespace pepp::debug::types;
  BoxedType u16 = _debug_info.typeInfo->box(types::Primitives::u16);
  BoxedType i16 = _debug_info.typeInfo->box(types::Primitives::i16);
  BoxedType u8 = _debug_info.typeInfo->box(types::Primitives::u8);
  _call = CommandFrame{.packets = {CommandPacket{
                           .modifiers = {}, .ops = {MemoryOp{.op = Opcodes::CALL, .name = "retAddr", .type = u16}}}}};
  _ret = CommandFrame{.packets = {CommandPacket{
                          .modifiers = {}, .ops = {MemoryOp{.op = Opcodes::RET, .name = "retAddr", .type = u16}}}}};
  _scall = CommandFrame{
      .packets = {CommandPacket{
          .modifiers = {},
          .ops = {FrameManagement{.op = Opcodes::ADD_FRAME}, MemoryOp{.op = Opcodes::PUSH, .name = "IR", .type = u8},
                  MemoryOp{.op = Opcodes::PUSH, .name = "SP", .type = u16},
                  MemoryOp{.op = Opcodes::PUSH, .name = "PC", .type = u16},
                  MemoryOp{.op = Opcodes::PUSH, .name = "X", .type = i16},
                  MemoryOp{.op = Opcodes::PUSH, .name = "A", .type = i16},
                  MemoryOp{.op = Opcodes::PUSH, .name = "NZVC", .type = u8}, FrameActive{.active = true}}}}};
  _sret = CommandFrame{.packets = {CommandPacket{.modifiers = {},
                                                 .ops = {FrameActive{.active = false},
                                                         MemoryOp{.op = Opcodes::POP, .name = "NZVC", .type = u8},
                                                         MemoryOp{.op = Opcodes::POP, .name = "A", .type = i16},
                                                         MemoryOp{.op = Opcodes::POP, .name = "X", .type = i16},
                                                         MemoryOp{.op = Opcodes::POP, .name = "PC", .type = u16},
                                                         MemoryOp{.op = Opcodes::POP, .name = "SP", .type = u16},
                                                         MemoryOp{.op = Opcodes::POP, .name = "IR", .type = u8},
                                                         FrameManagement{.op = Opcodes::REMOVE_FRAME}}}}};
}

void pepp::debug::StackTracer::clearStacks() {
  _lastSP = std::nullopt;
  _activeStack = nullptr;
  _stacks.clear();
}

void pepp::debug::StackTracer::update_volatile_values() {
  if (_env == nullptr) return;
  // Propogate dirtiness from volatiles to their parents.
  for (auto &ptr : _exprCache) {
    auto eval = ptr->evaluator();
    eval.cache().mark_clean(false);
    // Only attempt to re-evaluate if the term depends on volatiles.
    if (!eval.cache().depends_on_volatiles()) continue;
    auto old_v = eval.cache();
    auto new_v = eval.evaluate(CachePolicy::UseNonVolatiles, *_env);
    if (*old_v.value != new_v) eval.cache().mark_dirty();
  }
}

pepp::debug::StackTracer::const_iterator pepp::debug::StackTracer::cbegin() const { return _stacks.cbegin(); }

pepp::debug::StackTracer::const_iterator pepp::debug::StackTracer::cend() const { return _stacks.cend(); }

pepp::debug::StackTracer::const_iterator pepp::debug::StackTracer::begin() const { return _stacks.cbegin(); }

pepp::debug::StackTracer::const_iterator pepp::debug::StackTracer::end() const { return _stacks.cend(); }

pepp::debug::StackTracer::iterator pepp::debug::StackTracer::begin() { return _stacks.begin(); }

pepp::debug::StackTracer::iterator pepp::debug::StackTracer::end() { return _stacks.end(); }

std::size_t pepp::debug::StackTracer::size() const { return _stacks.size(); }

bool pepp::debug::StackTracer::empty() const { return size() == 0; }

const pepp::debug::Stack *pepp::debug::StackTracer::at(std::size_t index) const {
  if (index >= _stacks.size()) return nullptr;
  return _stacks[index].get();
}

namespace {
static const pepp::debug::CommandFrame call = pepp::debug::CommandFrame{.packets = {pepp::debug::CommandPacket{}}};
}

void pepp::debug::StackTracer::notifyInstruction(quint16 pc, quint16 spAfter, InstructionType type) {
  std::string cmds_str = "";
  std::optional<const pepp::debug::CommandFrame *> cf = std::nullopt;
  if (auto cmds = _debug_info.commands.find(pc); cmds != _debug_info.commands.end()) {
    cf = &cmds.value();
  }

  std::string sp_str = "";
  if (_lastSP) {
    qint16 diff = qint16(spAfter) - qint16(*_lastSP);
    sp_str = fmt::format("SP:{:04x}/{:<+5}", spAfter, diff);
  } else {
    sp_str = fmt::format("SP:{:04x}     ", spAfter);
  }
  quint16 spBefore = _lastSP.value_or(spAfter);
  _lastSP = spAfter;

  // Set to non-null if you want the active stack to be changed after processing stack commands.
  // If you want to change to stack *before* processing stack commands, do it yourself below.
  std::optional<decltype(spAfter)> futureStack = std::nullopt;
  std::string operation;
  switch (type) {
  case InstructionType::CALL:
    operation = "CALL";
    if (!cf) cf = &_call;
    break;
  case InstructionType::RET:
    operation = "RET";
    if (!cf) cf = &_ret;
    break;
  case InstructionType::TRAPRET:
    operation = "SRET";
    futureStack = spAfter;
    spAfter = spBefore + 10;
    if (!cf) cf = &_sret;
    break;
  case InstructionType::ALLOCATE: operation = "ALLOC"; break;
  case InstructionType::DEALLOCATE: operation = "DEALLOC"; break;
  case InstructionType::TRAP: {
    operation = "SCALL";
    spBefore = spAfter + 10;
    _activeStack = getOrAddStack(spBefore);
    if (!cf) cf = &_scall;
    break;
  }
  case InstructionType::ASSIGNMENT:
    operation = "SET";
    _activeStack = getOrAddStack(spAfter);
    if (!cf) cf = &_movasp;
    break;
  }
  if (cf) cmds_str = QString(*cf.value()).toStdString();

  _logger->info("{: <7} at PC:{:04x} {}  {}", operation, pc, sp_str, cmds_str);
  if (cf) {
    processCommandFrame(**cf, spBefore, spAfter, futureStack);
  } else { // TODO: mark self as invalid. We had no command for this PC, nor could we synthesize one.
  }
}

std::optional<const pepp::debug::Stack *> pepp::debug::StackTracer::stackAtAddress(quint32 addr) const {
  for (const auto &stack : _stacks)
    if (stack->contains(addr)) return stack.get();
  return std::nullopt;
}

std::optional<pepp::debug::Stack *> pepp::debug::StackTracer::stackAtAddress(quint32 addr) {
  for (auto &stack : _stacks)
    if (stack->contains(addr)) return stack.get();
  return std::nullopt;
}

pepp::debug::Stack *pepp::debug::StackTracer::getOrAddStack(quint32 address) {
  std::optional<pepp::debug::Stack *> stack = stackAtAddress(address);
  if (stack) return stack.value();
  auto new_stack = std::make_shared<pepp::debug::Stack>(address);
  new_stack->pushFrame();
  Q_ASSERT(new_stack->size() > 0);
  _stacks.push_back(new_stack);
  std::sort(_stacks.begin(), _stacks.end(),
            [](auto &lhs, auto &rhs) { return lhs->base_address() < rhs->base_address(); });
  return new_stack.get();
}

void pepp::debug::StackTracer::pushSlot(QString name, quint32 address, types::BoxedType type) {
  if (!_activeStack) _logger->warn("{: <4} No active stack to push to!", "");
  else if (auto tframe = _activeStack->top(); !tframe) _logger->warn("{: <4} Top frame is null", "");
  else {
    auto HexHint = pepp::debug::detail::UnsignedConstant::Format::Hex;
    auto addr = _exprCache.add_or_return(Constant((quint16)address, HexHint));
    auto memlookup = _exprCache.add_or_return(MemoryReadCastDeref(addr, type));
    if (_env) memlookup->evaluator().evaluate(CachePolicy::UseNever, *_env);
    quint32 size = types::bitness(unbox(type)) / 8;
    tframe->pushSlot(std::move(Slot(address, size, name, memlookup, tframe)));
  }
}

void pepp::debug::StackTracer::popSlot(quint16 expectedSize) {
  if (!_activeStack) _logger->warn("        No active stack to return from!");
  else if (auto tframe = _activeStack->top(); !tframe) _logger->warn("        Top frame is null");
  else if (auto trecord = tframe->top(); !trecord) _logger->warn("        Top frame record is null");
  else if (trecord->size() != expectedSize)
    _logger->warn("{: <4} Top frame record was {}B, expected {}B", "", trecord->size(), expectedSize);
  else tframe->popSlot();
}

void pepp::debug::StackTracer::processCommandFrame(const CommandFrame &frame, quint16 spBefore, quint16 spAfter,
                                                   std::optional<quint16> spFuture) {
  qint16 expectedDelta = spAfter - spBefore, actualDelta = 0;
  for (const auto &packet : frame.packets) {
    for (const auto &op : packet.ops) {
      switch (to_opcode(op)) {
      case Opcodes::INVALID: _logger->error("{: <6} Attempting to execute invalid command!", ""); return;
      case Opcodes::CALL: {
        if (!_activeStack) _logger->warn("{: <4} No active stack!", "");
        else if (auto tframe = _activeStack->top(); !tframe) _logger->warn("{: <4} Top frame is null", "");
        // If frame is already active, then call  initiates new frame AND marks it active.
        else if (tframe->active()) _activeStack->pushFrame().setActive(true);
        // Otherwise it just activates the frame
        else tframe->setActive(true);
        auto memop = std::get<MemoryOp>(op);
        quint16 size = types::bitness(unbox(memop.type)) / 8;
        spBefore -= size, actualDelta -= size;
        pushSlot(memop.name, spBefore, memop.type);
        _logger->info("{: <7} CALL'ed {} bytes", "", size);
        break;
      }
      case Opcodes::PUSH: {
        auto memop = std::get<MemoryOp>(op);
        quint16 size = types::bitness(unbox(memop.type)) / 8;
        spBefore -= size, actualDelta -= size;
        pushSlot(memop.name, spBefore, memop.type);
        _logger->info("{: <7} ALLOC'ed {} bytes", "", size);
        break;
      }
      case Opcodes::RET: {
        auto memop = std::get<MemoryOp>(op);
        quint16 size = types::bitness(unbox(memop.type)) / 8;
        popSlot(size);
        spBefore += size, actualDelta += size;
        _logger->info("{: <7} RET'ed {} bytes", "", size);
        if (!_activeStack) _logger->warn("{: <4} No active stack!", "");
        else if (auto tframe = _activeStack->top(); !tframe) _logger->warn("{: <4} Top frame is null", "");
        else if (tframe->empty()) _activeStack->popFrame();
        else tframe->setActive(false);
        break;
      }
      case Opcodes::POP: {
        auto memop = std::get<MemoryOp>(op);
        quint16 size = types::bitness(unbox(memop.type)) / 8;
        popSlot(size);
        spBefore += size, actualDelta += size;
        _logger->info("{: <7} DEALLOC'ed {} bytes", "", size);
        break;
      }

      case Opcodes::MARK_ACTIVE: {
        auto faop = std::get<FrameActive>(op);
        if (!_activeStack) _logger->warn("{: <4} No active stack!", "");
        else if (auto tframe = _activeStack->top(); !tframe) _logger->warn("{: <4} Top frame is null", "");
        else tframe->setActive(faop.active);
        _logger->info("{: <7} MARK_ACTIVE {}", "", faop.active);
        break;
      }
      case Opcodes::ADD_FRAME: {
        auto fmop = std::get<FrameManagement>(op);
        if (!_activeStack) _logger->warn("{: <4} No active stack!", "");
        else _activeStack->pushFrame();
        _logger->info("{: <7} PUSH_FRAME", "");
        break;
      }
      case Opcodes::REMOVE_FRAME: {
        auto fmop = std::get<FrameManagement>(op);
        if (!_activeStack) _logger->warn("{: <4} No active stack!", "");
        else if (auto tframe = _activeStack->top(); !tframe) _logger->warn("{: <4} Top frame is null", "");
        else if (!tframe->empty()) _logger->warn("{: <4} Top frame is not empty", "");
        else _activeStack->popFrame();
        _logger->info("{: <7} POP_FRAME", "");
        break;
      }
      }
    }
  }
  if (spFuture) {
    _activeStack = getOrAddStack(*spFuture);
    _logger->info("{: <7} SWITCH to SP:{:04x}", "", *spFuture);
  }
  _logger->info("\n{}", to_string(10));
  if (expectedDelta != actualDelta && !frame.packets.empty())
    _logger->warn("{: <4} delta SP of {:<+5}, expected {:<+5}", "", actualDelta, expectedDelta);
}

std::string pepp::debug::StackTracer::to_string(int left_pad) const {
  std::list<std::vector<std::string>> temp_stacks;
  std::size_t longest = 0;
  for (const auto &stack : _stacks) {
    auto vec = stack->to_string(0);
    longest = std::max(vec.size(), longest);
    temp_stacks.emplace(temp_stacks.begin(), std::move(vec));
  }
  if (longest == 0) return "";
  std::vector<std::string> joined_lines(longest);

  std::ranges::reverse(temp_stacks);

  for (const auto &temp_stack : temp_stacks) {
    int it = 0;
    for (const auto &line : temp_stack) {
      if (joined_lines[it].empty()) joined_lines[it++] = line;
      else joined_lines[it++] += std::string("    ") + line;
    }
    for (; it < joined_lines.size(); it++) joined_lines[it] += std::string(19 + 6, ' ');
  }
  std::string joiner = fmt::format("\n{}", std::string(left_pad, ' '));
  return fmt::format("{}{}\n", joiner, fmt::join(joined_lines.begin(), joined_lines.end(), joiner));
}

