#include "./pep_attributes.hpp"

pepp::tc::ir::attr::Type pepp::tc::ir::attr::Identifier::type() const { return TYPE; }

QStringView pepp::tc::ir::attr::Identifier::view() const { return value; }

QString pepp::tc::ir::attr::Identifier::to_string() const { return view().toString(); }

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

QString pepp::tc::ir::attr::SectionFlags::to_string() const {
  using namespace Qt::StringLiterals;
  static const QString rs = "r", ws = "w", xs = "x", zs = "z", e = "";
  return u"%1%2%3%4"_s.arg(r ? rs : e, w ? ws : e, x ? xs : e, z ? zs : e);
}
