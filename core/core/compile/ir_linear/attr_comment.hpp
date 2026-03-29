#pragma once

#include "core/compile/ir_linear/attr_identifier.hpp"

namespace pepp::tc {

struct Comment : public Identifier {
  static constexpr int TYPE = static_cast<int>(Type::Comment);
  int type() const override;
  Comment(std::string v);
};

struct CommentIndent : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::CommentIndent);
  int type() const override;
  enum class Level { Left, Right, Center } value = Level::Left;
};
} // namespace pepp::tc
