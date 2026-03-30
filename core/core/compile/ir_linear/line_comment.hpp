#pragma once

#include "core/compile/ir_linear/attr_comment.hpp"
#include "core/compile/ir_linear/line_base.hpp"
namespace pepp::tc {
struct CommentLine : public LinearIR {
  static constexpr int TYPE = static_cast<int>(LinearIRType::Comment);
  explicit CommentLine(Comment c) : comment(std::move(c)) {}
  const AAttribute *attribute(int type) const override;
  void insert(std::unique_ptr<AAttribute> attr) override;
  int type() const override;
  Comment comment;
};
} // namespace pepp::tc
