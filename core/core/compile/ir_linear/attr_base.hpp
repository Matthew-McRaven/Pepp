#pragma once
#include <memory>

namespace pepp::tc {
enum class Type {
  Invalid = 0,
  Identifier,
  Comment,
  CommentIndent,
  Argument,
  SymbolDeclaration,
  SectionFlags,
  FirstUser
};

struct AAttribute {
  virtual ~AAttribute() = default;
  virtual int type() const = 0;
};

struct AAttributeListNode {
  std::unique_ptr<AAttributeListNode> next = nullptr;
  std::unique_ptr<AAttribute> attr = nullptr;
};

} // namespace pepp::tc
