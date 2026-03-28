#include "./pep_attributes.hpp"
#include "fmt/format.h"

pepp::tc::ir::attr::Type pepp::tc::ir::attr::Identifier::type() const { return TYPE; }

std::string_view pepp::tc::ir::attr::Identifier::view() const { return value; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::Comment::type() const { return TYPE; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::CommentIndent::type() const { return TYPE; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::Pep10Mnemonic::type() const { return TYPE; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::Pep10AddrMode::type() const { return TYPE; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::Argument::type() const { return TYPE; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::SymbolDeclaration::type() const { return TYPE; }

pepp::tc::ir::attr::Type pepp::tc::ir::attr::SectionFlags::type() const { return TYPE; }

bool pepp::tc::ir::attr::SectionFlags::operator==(const SectionFlags &rhs) const {
  return r == rhs.r && w == rhs.w && x == rhs.x && z == rhs.z;
}

std::string pepp::tc::ir::attr::SectionFlags::to_string() const {
  static const std::string rs = "r", ws = "w", xs = "x", zs = "z", e = "";
  return fmt::format("{}{}{}{}", r ? rs : e, w ? ws : e, x ? xs : e, z ? zs : e);
}
