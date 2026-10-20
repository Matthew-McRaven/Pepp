#pragma once

#include "core/compile/ir_linear/attr_identifier.hpp"

namespace pepp::tc {
struct LinearIR;

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

// Disable start/end comments for a macro's expansion.
struct ExpansionComments : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::ExpansionComments);
  explicit ExpansionComments(bool value);
  int type() const override;
  bool value = true;
};

// Omit a line when formatting source and/or a listing. Without this attribute, a line is shown in both.
struct Hidden : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::Hidden);
  Hidden(bool source, bool listing);
  int type() const override;
  bool source = false, listing = false;
};
bool hidden_from_source(const LinearIR &line);
bool hidden_from_listing(const LinearIR &line);

// Omit a line's object code from the listing, but keep its address and source.
struct SuppressObjectCode : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::SuppressObjectCode);
  int type() const override;
};
bool object_code_suppressed(const LinearIR &line);
} // namespace pepp::tc
