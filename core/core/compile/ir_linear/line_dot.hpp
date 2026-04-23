#pragma once

#include "core/compile/ir_linear/attr_argument.hpp"
#include "core/compile/ir_linear/attr_identifier.hpp"
#include "core/compile/ir_linear/attr_section.hpp"
#include "core/compile/ir_linear/attr_symbol.hpp"
#include "core/compile/ir_linear/line_base.hpp"

namespace pepp::tc {
enum class DotCommands {
  ALIGN,
  ASCII,
  ASCIZ,
  BLOCK,
  BYTE,
  EQUATE,
  HALF,
  ORG,
  SECTION,
  WORD,
  IF,
  ELSEIF,
  ELSE,
  ENDIF,
  INLINE_MACRO,
  END_MACRO, // Not a real directive in our IR, but used as a marker for unmatched .endm
  FIRST_USER
};

struct DotAlign : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotAlign);
  enum class Which { ByteCount, Pow2 } which;
  explicit DotAlign(Which kind, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
  int type() const override;
  Argument argument;
};

struct DotLiteral : public LinearIR { // ASCII, byte, word
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotLiteral);
  enum class Which { ASCII, Byte1, Byte2, Byte4 } which;
  DotLiteral(Which kind, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
  int type() const override;
  Argument argument;
};

struct DotBlock : public LinearIR { // Block
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotBlock);
  explicit DotBlock(Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  std::optional<u64> object_size(u64 base_address) const override;
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

struct DotConditional : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::DotConditional);
  enum class Behavior { IF, ELSEIF, ELSE, ENDIF } behavior = Behavior::IF;
  // Use this ctor for ELSE/ENDIF
  explicit DotConditional(Behavior behavior);
  // Use this ctor for if/else.
  DotConditional(Behavior behavior, Argument arg);
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Argument argument;
};
} // namespace pepp::tc
