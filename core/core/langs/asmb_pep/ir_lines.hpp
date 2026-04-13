#pragma once
#include <memory>
#include <optional>
#include "core/compile/ir_linear/attr_argument.hpp"
#include "core/compile/ir_linear/line_base.hpp"
#include "core/compile/ir_linear/line_dot.hpp"
#include "core/langs/asmb_pep/ir_attributes.hpp"

namespace pepp::tc {
enum class PepIRType : int {
  Monadic = static_cast<int>(LinearIRType::FirstUser),
  Dyadic,
  DotAnnotate,
  MacroInvocation
};
enum class PepDotCommands : int {
  EXPORT = static_cast<int>(DotCommands::FIRST_USER),
  IMPORT,
  INPUT,
  OUTPUT,
  SCALL,
};
struct MonadicInstruction : public LinearIR {
  static constexpr int TYPE = static_cast<int>(PepIRType::Monadic);
  explicit MonadicInstruction(Pep10Mnemonic m) : mnemonic(m) {}
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
  int type() const override;
  Pep10Mnemonic mnemonic;
};

struct DyadicInstruction : public LinearIR {
  static constexpr int TYPE = static_cast<int>(PepIRType::Dyadic);
  DyadicInstruction(Pep10Mnemonic m, Pep10AddrMode am, Argument arg) : mnemonic(m), addr_mode(am), argument(arg) {}
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
  int type() const override;
  Pep10Mnemonic mnemonic;
  Pep10AddrMode addr_mode;
  Argument argument;
};

struct MacroInvocation : public LinearIR {};

struct DotAnnotate : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotAnnotate);
  enum class Which { EXPORT, IMPORT, INPUT, OUTPUT, SCALL } which;
  // Arg must always be an identifier
  DotAnnotate(Which dir, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Argument argument;
};

bool defines_symbol(const LinearIR &line);

} // namespace pepp::tc
