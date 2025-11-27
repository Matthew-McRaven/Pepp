#pragma once
#include <QtCore/qmetaobject.h>
#include <utility>
#include "./expr_rtti.hpp"
#include "exports.hpp"
namespace pepp::debug {
Q_NAMESPACE_EXPORT(PEPP_EXPORT);
enum class Opcodes : quint8 {
  INVALID = 0,
  PUSH,
  POP,
  CALL, // Special case of push
  RET,  // Special case of ret
  MARK_ACTIVE,
  ADD_FRAME,
  REMOVE_FRAME,
};
Q_ENUM_NS(Opcodes)

struct MemoryOp {
  Opcodes op; // Either PUSH, POP, CALL, or RET
  QString name;
  pepp::debug::types::BoxedType type;
  operator QString() const {
    static const auto meta = QMetaEnum::fromType<Opcodes>();
    const char *op_name = meta.valueToKey((int)op);
    return QString("%1 %2 as (%3)").arg(op_name).arg(name).arg(types::to_string(unbox(type)));
  }
  static zpp::bits::errc serialize(auto &archive, auto &self, pepp::debug::types::SerializationHelper &h) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive((quint8)self.op); errc != std::errc()) return errc;
      else if (errc = archive(self.name.toStdString()); errc != std::errc()) return errc;
      quint16 tmp = h.index_for_type(self.type);
      return archive(tmp);
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      quint8 tmp = 0;
      if (auto errc = archive(tmp); errc.code != std::errc()) return errc;
      self.op = static_cast<Opcodes>(tmp);
      std::string s;
      if (auto errc = archive(s); errc.code != std::errc()) return errc;
      self.name = QString::fromStdString(s);
      quint16 tmp2 = 0;
      if (auto errc = archive(tmp2); errc.code != std::errc()) return errc;
      self.type = h.type_for_index(tmp2);
      return std::errc();
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    else throw std::logic_error("Unreachable");
  }
};

struct FrameManagement {
  Opcodes op; // Either ADD_FRAME or REMOVE_FRAME
  operator QString() const {
    static const auto meta = QMetaEnum::fromType<Opcodes>();
    const char *op_name = meta.valueToKey((int)op);
    return QString("%1").arg(op_name);
  }
  static constexpr zpp::bits::errc serialize(auto &archive, auto &self, pepp::debug::types::SerializationHelper &h) {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) return archive((quint8)self.op);
    else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      quint8 tmp = 0;
      if (auto errc = archive(tmp); errc.code != std::errc()) return errc;
      self.op = static_cast<Opcodes>(tmp);
      return std::errc();
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    else throw std::logic_error("Unreachable");
  }
};

struct FrameActive {
  bool active; // true if the frame is active, false if it is inactive.
  operator QString() const {
    static const auto meta = QMetaEnum::fromType<Opcodes>();
    const char *op_name = meta.valueToKey((int)Opcodes::MARK_ACTIVE);
    return QString("%1 %2").arg(op_name).arg(active);
  }
  static constexpr zpp::bits::errc serialize(auto &archive, auto &self, pepp::debug::types::SerializationHelper &h) {
    return archive(self.active);
  }
};

using StackOp = std::variant<MemoryOp, FrameManagement, FrameActive>;
inline QString to_string(const StackOp &op) {
  return std::visit([](auto &&obj) -> QString { return QString(obj); }, op);
}

namespace detail {
struct ToOpcodeVisitor {
  Opcodes operator()(const FrameActive &) { return Opcodes::MARK_ACTIVE; }
  Opcodes operator()(const auto &obj) { return obj.op; }
};
} // namespace detail
inline Opcodes to_opcode(const StackOp &op) { return std::visit(detail::ToOpcodeVisitor{}, op); }

namespace detail {
template <typename T> struct SerializeVistor {
  pepp::debug::types::SerializationHelper &h;
  T &archive;
  template <typename U> std::errc operator()(U &v) { return v.serialize(archive, v, h); }
};
} // namespace detail

static zpp::bits::errc serialize_op(auto &archive, auto &type, pepp::debug::types::SerializationHelper &h) {
  using archive_type = std::remove_cvref_t<decltype(archive)>;
  if constexpr (archive_type::kind() == zpp::bits::kind::out) {
    if (auto errc = archive((quint8)type.index()); errc.code != std::errc()) return errc;
    return std::visit(detail::SerializeVistor{h, archive}, type);
  } else {
    // TODO: Read in the type
    quint8 index = 0;
    if (auto errc = archive(index); errc.code != std::errc()) return errc;
    switch (index) {
    case 0: {
      MemoryOp tmp;
      if (auto errc = tmp.serialize(archive, tmp, h); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    case 1: {
      FrameManagement tmp;
      if (auto errc = tmp.serialize(archive, tmp, h); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    case 2: {
      FrameActive tmp;
      if (auto errc = tmp.serialize(archive, tmp, h); errc.code != std::errc()) return errc;
      type = std::move(tmp);
      return std::errc();
    }
    }
    throw std::logic_error("Not implemented");
  }
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
  static auto serialize(auto &archive, auto &self, pepp::debug::types::SerializationHelper &h) -> zpp::bits::errc {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive((quint16)self.ops.size()); errc.code != std::errc()) return errc;
      for (const auto &op : self.ops) {
        if (auto errc = serialize_op(archive, op, h); errc.code != std::errc()) return errc;
      }
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      quint16 size = 0;
      if (auto errc = archive(size); errc.code != std::errc()) return errc;
      for (int i = 0; i < size; ++i) {
        StackOp op;
        if (auto errc = serialize_op(archive, op, h); errc.code != std::errc()) return errc;
        self.ops.push_back(std::move(op));
      }
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    else throw std::logic_error("Unreachable");
    return std::errc{};
  }
};
// At most one command frame should run per instruction.
// If you have multiple commands to execute for a single instruction, combine them into one frame.
struct CommandFrame {
  std::list<CommandPacket> packets;
  inline operator QString() const {
    QStringList packetStrs;
    packetStrs.reserve(static_cast<int>(this->packets.size()));
    for (const auto &packet : packets) packetStrs << QString(packet);
    return packetStrs.join(QStringLiteral("\n"));
  }

  static auto serialize(auto &archive, auto &self, pepp::debug::types::SerializationHelper &h) -> zpp::bits::errc {
    using archive_type = std::remove_cvref_t<decltype(archive)>;
    if constexpr (archive_type::kind() == zpp::bits::kind::out) {
      if (auto errc = archive((quint16)self.packets.size()); errc.code != std::errc()) return errc;
      for (const auto &packet : self.packets) {
        if (auto errc = packet.serialize(archive, packet, h); errc.code != std::errc()) return errc;
      }
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in && !std::is_const<decltype(self)>()) {
      quint16 size = 0;
      if (auto errc = archive(size); errc.code != std::errc()) return errc;
      for (int i = 0; i < size; ++i) {
        CommandPacket packet;
        if (auto errc = packet.serialize(archive, packet, h); errc.code != std::errc()) return errc;
        self.packets.push_back(std::move(packet));
      }
    } else if constexpr (archive_type::kind() == zpp::bits::kind::in) throw std::logic_error("Can't read into const");
    else throw std::logic_error("Unreachable");
    return std::errc{};
  }
};
} // namespace pepp::debug
