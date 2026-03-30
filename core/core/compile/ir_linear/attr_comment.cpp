#include "attr_comment.hpp"

int pepp::tc::Comment::type() const { return TYPE; }

pepp::tc::Comment::Comment(std::string v) : Identifier(v) {}

int pepp::tc::CommentIndent::type() const { return TYPE; }
