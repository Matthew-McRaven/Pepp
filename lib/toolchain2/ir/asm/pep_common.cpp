#include "pep_common.hpp"

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::LinearIR::attribute(attr::Type type) const {
  for (attr::ListNode *it = extended_attributes.get(); it != nullptr; it = it->next.get())
    if (it->attr->type() == type) return it->attr.get();
  return nullptr;
}

void pepp::tc::ir::LinearIR::insert(std::unique_ptr<attr::AAttribute> attr) {
  // Insert before that node, or when we reach the end of the list.
  auto *link = &extended_attributes;
  for (; *link != nullptr && (*link)->attr->type() < attr->type(); link = &(*link)->next);
  auto node = std::make_unique<attr::ListNode>();
  node->attr = std::move(attr);
  node->next = std::move(*link);
  *link = std::move(node);
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::CommentLine::attribute(attr::Type type) const {
  if (type == attr::Type::Comment) return &comment;
  else return LinearIR::attribute(type);
}

void pepp::tc::ir::CommentLine::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::Comment) comment = *(static_cast<attr::Comment *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::AddressableLine::attribute(attr::Type type) const {
  if (type == attr::Type::Address) return &address;
  else return LinearIR::attribute(type);
}

void pepp::tc::ir::AddressableLine::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::Address) address = *(static_cast<attr::Address *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::MonadicInstruction::attribute(attr::Type type) const {
  if (type == attr::Type::Mnemonic) return &mnemonic;
  else return AddressableLine::attribute(type);
}

void pepp::tc::ir::MonadicInstruction::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::Mnemonic) mnemonic = *(static_cast<attr::Pep10Mnemonic *>(attr.release()));
  else AddressableLine::insert(std::move(attr));
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::DyadicInstruction::attribute(attr::Type type) const {
  if (type == attr::Type::Mnemonic) return &mnemonic;
  else if (type == attr::Type::Argument) return &argument;
  else return AddressableLine::attribute(type);
}

void pepp::tc::ir::DyadicInstruction::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::Mnemonic) mnemonic = *(static_cast<attr::Pep10Mnemonic *>(attr.release()));
  if (attr->type() == attr::Type::AddressingMode) addr_mode = *(static_cast<attr::Pep10AddrMode *>(attr.release()));
  else if (attr->type() == attr::Type::Argument) argument = *(static_cast<attr::Argument *>(attr.release()));
  else AddressableLine::insert(std::move(attr));
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::DotLiteral::attribute(attr::Type type) const {
  if (type == attr::Type::Argument) return &argument;
  else return AddressableLine::attribute(type);
}

void pepp::tc::ir::DotLiteral::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::Argument) argument = *(static_cast<attr::Argument *>(attr.release()));
  else AddressableLine::insert(std::move(attr));
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::DotBlock::attribute(attr::Type type) const {
  if (type == attr::Type::Argument) return &argument;
  else return AddressableLine::attribute(type);
}

void pepp::tc::ir::DotBlock::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::Argument) argument = *(static_cast<attr::Argument *>(attr.release()));
  else AddressableLine::insert(std::move(attr));
}

const pepp::tc::ir::attr::AAttribute *pepp::tc::ir::DotEquate::attribute(attr::Type type) const {
  if (type == attr::Type::SymbolDeclaration) return &symbol;
  else if (type == attr::Type::Argument) return &argument;
  else return LinearIR::attribute(type);
}

void pepp::tc::ir::DotEquate::insert(std::unique_ptr<attr::AAttribute> attr) {
  if (attr->type() == attr::Type::SymbolDeclaration) symbol = *(static_cast<attr::SymbolDeclaration *>(attr.release()));
  else if (attr->type() == attr::Type::Argument) argument = *(static_cast<attr::Argument *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

bool pepp::tc::ir::defines_symbol(const LinearIR &line) {
  auto sym = line.attribute(attr::Type::SymbolDeclaration);
  return sym != nullptr;
}
