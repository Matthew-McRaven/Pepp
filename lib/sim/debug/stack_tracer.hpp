#pragma once
#include <spdlog/logger.h>
#include "./stack_layout.hpp"
#include "expr_eval.hpp"
#include "expr_parser.hpp"
#include "toolchain/pas/obj/trace_tags.hpp"
namespace pepp::debug {

/*
 * This can take three forms relevant to stack tracing:
 *   (a) Replace a stack modifying instruction with a different instruction
 *   (b) Replace a non-stack instruction with a stack instruction
 *   (c) Modify the number of bytes de/allocated via operand specifier replacement
 *
 * Cases (a) and (b) are covered by the simulator posting events to the stack tracing SW on stack modifying
 * instructions. The event could be compared against the trace commands. If the trace command does not make sense for
 * the instruction (e.g., deactivate a frame on an alloc), we can halt tracing / enter an invalid state. Alternatively,
 * if there is no event, we can enter an invalid state.
 *
 * These methods do not cover (c), since the tracer is unaware of the number of bytes actually allocated.
 * So, the simulator needs to notify the debugger of the new SP after each instruction.
 * We can compare the number of bytes actually de/allocated to the trace command to catch errors.
 */
class StackTracer {
  pepp::debug::Environment *_env = nullptr;
  pas::obj::common::DebugInfo _debug_info;
  using container = std::vector<std::shared_ptr<Stack>>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  explicit StackTracer();
  bool canTrace() const;
  pas::obj::common::DebugInfo const &debugInfo() const;
  void setDebugInfo(pas::obj::common::DebugInfo debug_info, pepp::debug::Environment *env);
  // If debug info is re-used between simulations, call this to clear state.
  void clearStacks();
  // Call at the same time as WatchExpressionEditor::update_volatile_values
  void update_volatile_values();

  std::optional<Stack const *> stackAtAddress(quint32) const;

  const_iterator cbegin() const;
  const_iterator cend() const;
  const_iterator begin() const;
  const_iterator end() const;
  iterator begin();
  iterator end();
  std::size_t size() const;
  bool empty() const;
  const Stack *at(std::size_t index) const;

  enum class InstructionType { CALL, RET, TRAP, TRAPRET, ALLOCATE, DEALLOCATE, ASSIGNMENT };
  // PC of the instruction that is being executed. SP is the sp *after* the instruction was executed.
  // I need the modified SP so that I can check that the # of bytes de/allocated matches the debug info.
  // This is not very "physical", because I am examining processor state mid-instruction from the debugger.
  void notifyInstruction(quint16 pc, quint16 spAfter, InstructionType type);

private:
  std::optional<Stack *> stackAtAddress(quint32);
  Stack *getOrAddStack(quint32 address);
  void pushSlot(QString name, quint32 address, pepp::debug::types::BoxedType type);
  void popSlot(quint16 expectedSize);
  // spBefore is prior to executing the instruction, spAfter is after executing it.
  // spFuture is the stack which we should switch to after processing this command frame.
  void processCommandFrame(const pepp::debug::CommandFrame &, quint16 spBefore, quint16 spAfter,
                           std::optional<quint16> spFuture);
  std::string to_string(int left_pad) const;
  std::shared_ptr<spdlog::logger> _logger;
  // Keep stacks sorted by base address so we can do a binary search.
  pepp::debug::ExpressionCache _exprCache;
  container _stacks;
  std::optional<quint16> _lastSP;
  // Cache the last active stack to avoid searching every time.
  Stack *_activeStack = nullptr;
  pepp::debug::CommandFrame _call, _ret, _scall, _sret, _movasp;
};
} // namespace pepp::debug
