#include "core/compile/ir_linear/line_dot.hpp"
#include "core/compile/ir_value/base.hpp"
#include "core/macros.hpp"

pepp::tc::DotAlign::DotAlign(Argument arg) : argument(arg) {}

const pepp::tc::AAttribute *pepp::tc::DotAlign::attribute(int type) const {
  if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::DotAlign::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

std::optional<u16> pepp::tc::DotAlign::object_size(u16 base_address) const {
  u16 align = argument.value->value_as<u16>();
  auto ret = (align - (base_address % align)) % align;
  // If return value would be 0, choose nullopt to prevent useless address in listing.
  return ret == 0 ? std::nullopt : std::optional<u16>(ret);
  // if (direction == Direction::Forward) return (align - (base_address % align)) % align;
  // else if (direction == Direction::Backward) return base_address % align;
  // else return 0;
}

int pepp::tc::DotAlign::type() const { return TYPE; }

const pepp::tc::AAttribute *pepp::tc::DotLiteral::attribute(int type) const {
  if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

pepp::tc::DotLiteral::DotLiteral(Which kind, Argument arg) : which(kind), argument(arg) {}

void pepp::tc::DotLiteral::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

std::optional<u16> pepp::tc::DotLiteral::object_size(u16) const {
  switch (which) {
  case Which::ASCII: return argument.value->serialized_size();
  case Which::Byte: return 1;
  case Which::Word: return 2;
  }

  PEPP_UNREACHABLE();
}

int pepp::tc::DotLiteral::type() const { return TYPE; }

const pepp::tc::AAttribute *pepp::tc::DotBlock::attribute(int type) const {
  if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

pepp::tc::DotBlock::DotBlock(Argument arg) : argument(arg) {}

void pepp::tc::DotBlock::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

std::optional<u16> pepp::tc::DotBlock::object_size(u16) const { return argument.value->value_as<u16>(); }

int pepp::tc::DotBlock::type() const { return TYPE; }

pepp::tc::DotEquate::DotEquate(SymbolDeclaration symbol, Argument arg) : symbol(symbol), argument(arg) {}

const pepp::tc::AAttribute *pepp::tc::DotEquate::attribute(int type) const {
  if (type == SymbolDeclaration::TYPE) return &symbol;
  else if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::DotEquate::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == SymbolDeclaration::TYPE) symbol = *(static_cast<SymbolDeclaration *>(attr.release()));
  else if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::DotEquate::type() const { return TYPE; }

pepp::tc::DotSection::DotSection(Identifier name, SectionFlags flags) : name(name), flags(flags) {}

const pepp::tc::AAttribute *pepp::tc::DotSection::attribute(int type) const {
  if (type == Identifier::TYPE) return &name;
  else if (type == SectionFlags::TYPE) return &flags;
  else return LinearIR::attribute(type);
}

void pepp::tc::DotSection::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Identifier::TYPE) name = *(static_cast<Identifier *>(attr.release()));
  else if (attr->type() == SectionFlags::TYPE) flags = *(static_cast<SectionFlags *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::DotSection::type() const { return TYPE; }

pepp::tc::DotOrg::DotOrg(Behavior behavior, Argument arg) : behavior(behavior), argument(arg) {}

const pepp::tc::AAttribute *pepp::tc::DotOrg::attribute(int type) const {
  if (type == Argument::TYPE) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::DotOrg::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == Argument::TYPE) argument = *(static_cast<Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::DotOrg::type() const { return TYPE; }
