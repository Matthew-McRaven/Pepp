#pragma once
#include <spdlog/logger.h>
#include "toolchain/pas/obj/trace_tags.hpp"
namespace pepp::debug {
class Record {
public:
  quint32 address, size;
  QString name;
};
/*
class Frame {
  using container = std::vector<Record>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  bool active() const { return _active; }
  void setActive(bool active) { _active = active; }
  void pushRecord(Record &&record) { _records.push_back(std::move(record)); }

  quint32 base_address() const { return _baseAddress; }
  quint32 top_address() const {
    quint32 address = 0;
    for (const auto &it : std::as_const(_records)) address = qMax(address, it.address + it.size);
    return address;
  }
  const_iterator cbegin() const { return _records.cbegin(); }
  const_iterator cend() const { return _records.cend(); }
  iterator begin() { return _records.begin(); }
  iterator end() { return _records.end(); }
  const_iterator begin() const { return _records.cbegin(); }
  const_iterator end() const { return _records.cend(); }
  std::size_t size() const { return _records.size(); }
  bool empty() const { return _records.empty(); }

private:
  container _records = {};
  bool _active = false;
  quint32 _baseAddress = -1;
};

class Stack {
  using container = std::vector<Frame>;
  using iterator = typename container::iterator;
  using const_iterator = typename container::const_iterator;

public:
  Frame *top() {
    if (_frames.size() == 0) return nullptr;
    return &_frames.back();
  }
  quint32 base_address() const { return _baseAddress; }
  quint32 top_address() const {
    quint32 address = 0;
    for (const auto &it : std::as_const(_frames)) address = qMax(address, it.top_address());
    return address;
  }
  const_iterator cbegin() const { return _frames.cbegin(); }
  const_iterator cend() const { return _frames.cend(); }
  iterator begin() { return _frames.begin(); }
  iterator end() { return _frames.end(); }
  const_iterator begin() const { return _frames.cbegin(); }
  const_iterator end() const { return _frames.cend(); }
  std::size_t size() const { return _frames.size(); }
  bool empty() const { return _frames.empty(); }

private:
  container _frames;
  quint32 _baseAddress = -1;
};
*/
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
  pas::obj::common::DebugInfo _debug_info;
  // using container = std::vector<std::shared_ptr<Stack>>;
  // using iterator = typename container::iterator;
  // using const_iterator = typename container::const_iterator;

public:
  explicit StackTracer();
  bool canTrace() const { return true; /*!_debug_info.commands.empty() && _debug_info.typeInfo != nullptr;*/ }
  inline void setDebugInfo(pas::obj::common::DebugInfo debug_info) {
    _debug_info = debug_info;
    _lastSP = std::nullopt;
  }
  pas::obj::common::DebugInfo const &debugInfo() const { return _debug_info; }
  // Stack &activeStack();
  // Stack const &activeStack() const;
  // std::optional<Stack const *> stackAtAddress(quint32) const;
  void createEmptyStack(quint32 address);
  void activateStack(quint32 address);
  void activateStack(std::size_t index);

  // const_iterator cbegin() const { return _stacks.cbegin(); }
  // const_iterator cend() const { return _stacks.cend(); }
  // iterator begin() { return _stacks.begin(); }
  // iterator end() { return _stacks.end(); }
  // const_iterator begin() const { return _stacks.cbegin(); }
  // const_iterator end() const { return _stacks.cend(); }
  std::size_t size() const { return 0; /*_stacks.size();*/ }
  bool empty() const { return size() == 0; }
  enum class InstructionType { CALL, RET, TRAP, TRAPRET, ALLOCATE, DEALLOCATE, ASSIGNMENT };
  // PC of the instruction that is being executed. SP is the sp *after* the instruction was executed.
  // I need the modified SP so that I can check that the # of bytes de/allocated matches the debug info.
  // This is not very "physical", because I am examining processor state mid-instruction from the debugger.
  void notifyInstruction(quint16 pc, quint16 spAfter, InstructionType type);

private:
  std::shared_ptr<spdlog::logger> _logger;
  // container _stacks;
  std::optional<quint16> _lastSP;
  std::size_t _activeIndex = 0;
};
} // namespace pepp::debug
