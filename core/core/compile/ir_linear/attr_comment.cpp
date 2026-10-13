#include "attr_comment.hpp"
#include "core/compile/ir_linear/line_base.hpp"

int pepp::tc::Comment::type() const { return TYPE; }

pepp::tc::Comment::Comment(std::string v) : Identifier(v) {}

int pepp::tc::CommentIndent::type() const { return TYPE; }

pepp::tc::ExpansionComments::ExpansionComments(bool value) : value(value) {}

int pepp::tc::ExpansionComments::type() const { return TYPE; }

pepp::tc::Hidden::Hidden(bool source, bool listing) : source(source), listing(listing) {}

int pepp::tc::Hidden::type() const { return TYPE; }

bool pepp::tc::hidden_from_source(const LinearIR &line) {
  const auto *hidden = line.typed_attribute<Hidden>();
  return hidden && hidden->source;
}

bool pepp::tc::hidden_from_listing(const LinearIR &line) {
  const auto *hidden = line.typed_attribute<Hidden>();
  return hidden && hidden->listing;
}
