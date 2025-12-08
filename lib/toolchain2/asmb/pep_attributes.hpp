#pragma once

#include "enums/isa/pep10.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/ast/value/symbolic.hpp"
#include "toolchain/symbol/entry.hpp"
#include "toolchain2/support/allocators/string_pool.hpp"

namespace pepp::tc::ir::attr {

enum class Type {
  Invalid = 0,
  Identifier,
  Comment,
  CommentIndent,
  Mnemonic,
  AddressingMode,
  Argument,
  SymbolDeclaration,
  SectionFlags
};

struct AAttribute {
  virtual ~AAttribute() = default;
  virtual Type type() const = 0;
};

struct Identifier : public AAttribute {
  static constexpr Type TYPE = Type::Identifier;
  Type type() const override;
  Identifier(support::StringPool *pool, support::PooledString id) : pool(pool), id(id) {}
  support::StringPool *pool = nullptr;
  support::PooledString id;
  QStringView view() const;
  QString to_string() const;
};

struct Comment : public Identifier {
  static constexpr Type TYPE = Type::Comment;
  Type type() const override;
  Comment(support::StringPool *pool, support::PooledString id) : Identifier(pool, id) {}
};

struct CommentIndent : public AAttribute {
  static constexpr Type TYPE = Type::CommentIndent;
  Type type() const override;
  enum class Level { Left, Right, Center } value = Level::Left;
};

struct Pep10Mnemonic : public AAttribute {
  static constexpr Type TYPE = Type::Mnemonic;
  Type type() const override;
  Pep10Mnemonic(isa::Pep10::Mnemonic instruction) : instruction(instruction) {}
  isa::Pep10::Mnemonic instruction = isa::Pep10::Mnemonic::INVALID;
};

struct Pep10AddrMode : public AAttribute {
  static constexpr Type TYPE = Type::AddressingMode;
  Type type() const override;
  Pep10AddrMode(isa::Pep10::AddressingMode mode) : addr_mode(mode) {}
  isa::Pep10::AddressingMode addr_mode = isa::Pep10::AddressingMode::INVALID;
};

struct Argument : public AAttribute {
  static constexpr Type TYPE = Type::Argument;
  Type type() const override;
  Argument(std::shared_ptr<pas::ast::value::Base> value) : value(std::move(value)) {}
  std::shared_ptr<pas::ast::value::Base> value;
};

struct SymbolDeclaration : public AAttribute {
  static constexpr Type TYPE = Type::SymbolDeclaration;
  Type type() const override;
  SymbolDeclaration(QSharedPointer<symbol::Entry> entry) : entry(entry) {}
  QSharedPointer<symbol::Entry> entry;
};

struct SectionFlags : public AAttribute {
  static constexpr Type TYPE = Type::SectionFlags;
  Type type() const override;
  SectionFlags(bool r, bool w, bool x, bool z) : r(r), w(w), x(x), z(z) {}
  // Must update == if flags changes. Cannot use default due to abstract base class.
  bool r = false, w = false, x = false, z = false;
  bool operator==(const SectionFlags &rhs) const;
  QString to_string() const;
};

// Intentionally NOT an AAttribute, because I do not want it stored in my primary IR.
// I want it sotred in a side table
struct Address {
  Address(quint16 address, quint16 size) : address(address), size(size) {}
  quint16 address = 0, size = 0;
};

struct ListNode {
  std::unique_ptr<ListNode> next = nullptr;
  std::unique_ptr<AAttribute> attr = nullptr;
};
} // namespace pepp::tc::ir::attr
