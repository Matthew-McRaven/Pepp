#pragma once

#include <string>

#include "SvgElement.hpp"

class SvgCDataElement final : public Cloneable<SvgCDataElement, SvgElement>
{
public:
  SvgType::Type TYPE = SvgType::Type::SvgCDataElement;

  SvgCDataElement();
  explicit SvgCDataElement(const std::string &value);
  ~SvgCDataElement() = default;
  // Copy and move
  SvgCDataElement(const SvgCDataElement &) = default;
  SvgCDataElement &operator=(const SvgCDataElement &) = default;
  SvgCDataElement(SvgCDataElement &&) noexcept = default;
  SvgCDataElement &operator=(SvgCDataElement &&) noexcept = default;

  //  Called by base class
  SvgType::Type elementType() const override;
  void toXml(SvgRope &output) const override;
  bool setAttribute(const std::string &key, const std::string &value) override;
};
