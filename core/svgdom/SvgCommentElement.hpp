#pragma once

#include <string>

#include "SvgElement.hpp"

class SvgCommentElement final : public Cloneable<SvgCommentElement, SvgElement>
{
public:
  SvgType::Type TYPE = SvgType::Type::SvgCommentElement;

  SvgCommentElement();
  explicit SvgCommentElement(const std::string &comment);
  ~SvgCommentElement() = default;
  // Can copy and move
  SvgCommentElement(const SvgCommentElement &) = default;
  SvgCommentElement &operator=(const SvgCommentElement &) = default;
  SvgCommentElement(SvgCommentElement &&) noexcept = default;
  SvgCommentElement &operator=(SvgCommentElement &&) noexcept = default;

  //  Called by base class
  SvgType::Type elementType() const override;
  void toXml(SvgRope &output) const override;
  bool setAttribute(const std::string &key, const std::string &value) override;
};
