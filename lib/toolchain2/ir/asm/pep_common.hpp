#pragma once
#include "./attributes.hpp"
#include "toolchain2/support/source/location.hpp"

namespace pepp::tc::ir {
struct LinearIR {
  virtual ~LinearIR() = default;

  // Searches the linked list of attributes for one of the given type.
  // Must override if you inline an attribute into future IR lines.
  virtual const attr::AAttribute *attribute(attr::Type type) const;

  // Override this method if you inline an attribute into future IR lines.
  virtual void insert(std::unique_ptr<attr::AAttribute> attr);

  template <typename Attribute> const Attribute *typed_attribute(attr::Type type) const {
    static_assert(std::is_base_of_v<attr::AAttribute, Attribute>, "Attribute must derive from attr::AAttribute");
    return dynamic_cast<const Attribute *>(getAttribute(Attribute::TYPE));
  }
  support::LocationInterval source_interval;
  // Head of linked list of additional attributes
  std::unique_ptr<attr::ListNode> extended_attributes;
};

struct EmptyLine : public LinearIR {};

struct CommentLine : public LinearIR {
  CommentLine(attr::Comment c) : comment(std::move(c)) {}
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::Comment comment;
};

struct AddressableLine : public LinearIR {
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::Address address;
};

struct MonadicInstruction : public AddressableLine {
  MonadicInstruction(attr::Pep10Mnemonic m) : mnemonic(m) {}
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::Pep10Mnemonic mnemonic;
};

struct DyadicInstruction : public AddressableLine {
  DyadicInstruction(attr::Pep10Mnemonic m, attr::Pep10AddrMode am, attr::Argument arg)
      : mnemonic(m), addr_mode(am), argument(arg) {}
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::Pep10Mnemonic mnemonic;
  attr::Pep10AddrMode addr_mode;
  attr::Argument argument;
};

struct DotLiteral : public AddressableLine { // ASCII, byte, word
  enum class Which { ASCII, Byte, Word } which;
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::Argument argument;
};

struct DotBlock : public AddressableLine { // Block
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::Argument argument;
};

struct DotEquate : public LinearIR {
  const attr::AAttribute *attribute(attr::Type type) const override;
  void insert(std::unique_ptr<attr::AAttribute> attr) override;
  attr::SymbolDeclaration symbol;
  attr::Argument argument;
};
struct DotAlign : public AddressableLine {};
struct DotSection : public LinearIR {};
struct DotSCall : public LinearIR {};
struct DotImportExport : public LinearIR {};
struct DotInputOutput : public LinearIR {};
struct DotOrg : public LinearIR {
  enum class Behavior { BURN, ORG } behavior = Behavior::ORG;
};
struct MacroInvocation : public LinearIR {};

bool defines_symbol(const LinearIR &line);

} // namespace pepp::tc::ir
