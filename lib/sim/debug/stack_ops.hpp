#pragma once
#include <QtCore/qmetaobject.h>
#include "./expr_rtti.hpp"
namespace pepp::debug {
Q_NAMESPACE
enum class Opcodes : quint8 {
  INVALID = 0,
  PUSH,
  POP,
  MARK_ACTIVE,
  ADD_FRAME,
  REMOVE_FRAME,
};
Q_ENUM_NS(Opcodes)

struct MemoryOp {
  Opcodes op; // Either PUSH or Pop
  // Indirect handle gives us the name, direct gives us type info.
  types::TypeInfo::IndirectHandle name;
  types::TypeInfo::DirectHandle type;
  operator QString() const {
    static const auto meta = QMetaEnum::fromType<Opcodes>();
    const char *op_name = meta.valueToKey((int)op);
    return QString("%1 %2, %3").arg(op_name).arg(QString(name)).arg(QString(type));
  }
};

struct FrameManagement {
  Opcodes op; // Either ADD_FRAME or REMOVE_FRAME
  operator QString() const {
    static const auto meta = QMetaEnum::fromType<Opcodes>();
    const char *op_name = meta.valueToKey((int)op);
    return QString("%1").arg(op_name);
  }
};

struct FrameActive {
  bool active; // true if the frame is active, false if it is inactive.
  operator QString() const {
    static const auto meta = QMetaEnum::fromType<Opcodes>();
    const char *op_name = meta.valueToKey((int)Opcodes::MARK_ACTIVE);
    return QString("%1 %2").arg(op_name).arg(active);
  }
};

using StackOp = std::variant<MemoryOp, FrameManagement, FrameActive>;
inline QString to_string(const StackOp &op) {
  return std::visit([](auto &&obj) -> QString { return QString(obj); }, op);
}

// Used to store the translation of a single command like @params#my#names#here
// Which will decompose into multiple stack ops
struct CommandPacket {
  std::list<void *> modifiers;
  std::vector<StackOp> ops;
  inline operator QString() const {
    QStringList opStrings;
    opStrings.reserve(static_cast<int>(ops.size()));
    for (const auto &op : ops) opStrings << to_string(op);
    return opStrings.join(QStringLiteral("; "));
  }
};
// At most one command frame should run per instruction.
// If you have multiple commands to execute for a single instruction, combine them into one frame.
struct CommandFrame {
  std::list<CommandPacket> commands;
  inline operator QString() const {
    QStringList packets;
    packets.reserve(static_cast<int>(commands.size()));
    for (const auto &packet : commands) packets << QString(packet);
    return packets.join(QStringLiteral("\n"));
  }
};
} // namespace pepp::debug
