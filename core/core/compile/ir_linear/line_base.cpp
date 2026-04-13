#include "core/compile/ir_linear/line_base.hpp"
#include <utility>
#include "core/compile/ir_linear/attr_base.hpp"

const pepp::tc::AAttribute *pepp::tc::LinearIR::attribute(int type) const {
  for (AAttributeListNode *it = extended_attributes.get(); it != nullptr; it = it->next.get())
    if (it->attr->type() == type) return it->attr.get();
  return nullptr;
}

pepp::tc::AAttribute *pepp::tc::LinearIR::attribute(int type) {
  return const_cast<AAttribute *>(std::as_const(*this).attribute(type));
}

void pepp::tc::LinearIR::insert(std::unique_ptr<AAttribute> attr) {
  // Insert before that node, or when we reach the end of the list.
  auto *link = &extended_attributes;
  for (; *link != nullptr && (*link)->attr->type() < attr->type(); link = &(*link)->next);
  auto node = std::make_unique<AAttributeListNode>();
  node->attr = std::move(attr);
  node->next = std::move(*link);
  *link = std::move(node);
}

std::optional<u64> pepp::tc::LinearIR::object_size(u64) const { return std::nullopt; }
