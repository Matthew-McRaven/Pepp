#pragma once

#include "core/compile/ir_linear/attr_argument.hpp"
#include "core/compile/ir_linear/attr_identifier.hpp"
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_base.hpp"

namespace pepp::tc {
enum class DotCommands { ALIGN, ASCII, BLOCK, BYTE, EQUATE, HALF, ORG, SECTION, WORD, FIRST_USER };

struct DotAlign : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotAlign);
  explicit DotAlign(Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u16> object_size(u16 base_address) const override;
  int type() const override;
  Argument argument;
};

struct DotLiteral : public LinearIR { // ASCII, byte, word
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotLiteral);
  enum class Which { ASCII, Byte1, Byte2, Byte4 } which;
  DotLiteral(Which kind, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u16> object_size(u16 base_address) const override;
  int type() const override;
  Argument argument;
};

struct DotBlock : public LinearIR { // Block
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotBlock);
  explicit DotBlock(Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u16> object_size(u16 base_address) const override;
  int type() const override;
  Argument argument;
};

struct DotEquate : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotEquate);
  DotEquate(SymbolDeclaration symbol, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  SymbolDeclaration symbol;
  Argument argument;
};

struct DotSection : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotSection);
  DotSection(Identifier name, SectionFlags flags);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Identifier name;
  SectionFlags flags;
};

struct DotOrg : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotOrg);
  enum class Behavior { BURN, ORG } behavior = Behavior::ORG;
  // Arg must always be an number
  DotOrg(Behavior behavior, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Argument argument;
};
} // namespace pepp::tc
