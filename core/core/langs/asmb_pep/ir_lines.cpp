#include "core/langs/asmb_pep/ir_lines.hpp"
#include <utility>
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_value/base.hpp"
#include "core/langs/asmb_pep/ir_visitor.hpp"
#include "core/macros.hpp"

const pepp::tc::AAttribute *pepp::tc::MonadicInstruction::attribute(int type) const {
  if (type == Pep10Mnemonic::TYPE) return &mnemonic;
  else return LinearIR::attribute(type);
}

void pepp::tc::MonadicInstruction::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Pep10Mnemonic::TYPE) mnemonic = *(static_cast<Pep10Mnemonic *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

std::optional<u64> pepp::tc::MonadicInstruction::object_size(u64 base_address) const { return 1; }

int pepp::tc::MonadicInstruction::type() const { return TYPE; }

const pepp::tc::AAttribute *pepp::tc::DyadicInstruction::attribute(int type) const {
  if (type == Pep10Mnemonic::TYPE) return &mnemonic;
  else if (type == Pep10AddrMode::TYPE) return &argument;
  else if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::DyadicInstruction::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Pep10Mnemonic::TYPE) mnemonic = *(static_cast<Pep10Mnemonic *>(attr.release()));
  if (attr->type() == Pep10AddrMode::TYPE) addr_mode = *(static_cast<Pep10AddrMode *>(attr.release()));
  else if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

std::optional<u64> pepp::tc::DyadicInstruction::object_size(u64) const { return 3; }

int pepp::tc::DyadicInstruction::type() const { return TYPE; }

pepp::tc::DotAnnotate::DotAnnotate(Which which, Argument arg) : which(which), argument(arg) {}

const pepp::tc::AAttribute *pepp::tc::DotAnnotate::attribute(int type) const {
  if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::DotAnnotate::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::DotAnnotate::type() const { return TYPE; }

bool pepp::tc::defines_symbol(const LinearIR &line) {
  auto sym = line.attribute(SymbolDeclaration::TYPE);
  return sym != nullptr;
}

bool pepp::tc::allows_symbol(const LinearIR &line) {
  switch (line.type()) {
  case (int)LinearIRType::Empty: return false;
  case (int)LinearIRType::Comment: return false;
  case (int)LinearIRType::DotAlign: return true;
  case (int)LinearIRType::DotSymbol: return true;
  case (int)LinearIRType::DotBlock: return true;
  case (int)LinearIRType::DotEquate: return true;
  case (int)LinearIRType::DotLiteral: return true;
  case (int)LinearIRType::DotOrg: return false;
  case (int)LinearIRType::DotSection: return false;
  case (int)LinearIRType::DotConditional: return false;
  case (int)LinearIRType::MacroInstantiation: return true;
  case (int)LinearIRType::MacroDefinition: return false;
  case (int)PepIRType::Monadic: return true;
  case (int)PepIRType::Dyadic: return true;
  case (int)PepIRType::DotAnnotate: return false;
  default: return false;
  }
}
