#pragma once

#include "enums/isa/pep10.hpp"
#include "toolchain/pas/ast/value/base.hpp"
#include "toolchain/pas/ast/value/symbolic.hpp"
#include "toolchain/symbol/entry.hpp"
#include "toolchain2/support/allocators/string_pool.hpp"

namespace pepp::tc::ir::attr {

enum class Type { Invalid = 0, Comment, CommentIndent, Address, Mnemonic, AddressingMode, Argument, SymbolDeclaration };

struct AAttribute {
  virtual ~AAttribute() = default;
  virtual Type type() const = 0;
};

struct Comment : public AAttribute {
  static constexpr Type TYPE = Type::Comment;
  Type type() const override;
  Comment(support::StringPool *pool, support::PooledString id) : pool(pool), id(id) {}
  support::StringPool *pool = nullptr;
  support::PooledString id;
};

struct CommentIndent : public AAttribute {
  static constexpr Type TYPE = Type::CommentIndent;
  Type type() const override;
  enum class Level { Left, Right, Center } value = Level::Left;
};

struct Address : public AAttribute {
  static constexpr Type TYPE = Type::Address;
  Type type() const override;
  uint16_t address = 0;
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

struct ListNode {
  std::unique_ptr<ListNode> next = nullptr;
  std::unique_ptr<AAttribute> attr = nullptr;
};
} // namespace pepp::tc::ir::attr
