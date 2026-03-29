#include "core/compile/ir_linear/line_comment.hpp"

const pepp::tc::AAttribute *pepp::tc::CommentLine::attribute(int type) const {
  if (type == Comment::TYPE) return &comment;
  else return LinearIR::attribute(type);
}

void pepp::tc::CommentLine::insert(std::unique_ptr<AAttribute> attr) {
  if (attr->type() == CommentLine::TYPE) comment = *(static_cast<Comment *>(attr.release()));
  else LinearIR::insert(std::move(attr));
}

int pepp::tc::CommentLine::type() const { return TYPE; }
