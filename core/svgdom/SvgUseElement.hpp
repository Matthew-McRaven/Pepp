#pragma once

#include <string>

#include "SvgElement.hpp"
#include "utility_p.hpp"

class SvgUseElement final : public Cloneable<SvgUseElement, SvgElement>
{
    std::string _href;

public:
  static constexpr SvgType::Type TYPE = SvgType::Type::SvgUseElement;

  SvgUseElement();
  explicit SvgUseElement(const std::string &name, const std::string &value = "");
  ~SvgUseElement() = default;
  // Allo copy and move
  SvgUseElement(const SvgUseElement &) = default;
  SvgUseElement &operator=(const SvgUseElement &) = default;
  SvgUseElement(SvgUseElement &&) noexcept = default;
  SvgUseElement &operator=(SvgUseElement &&) noexcept = default;

  //  Value does not apply to SvgUseElement
  const std::string &value() const override;
  void setValue(const std::string &value) override;

  //	User access functions
  SvgType::Type elementType() const override;

  //  Values can be changed, but not units of measure (yet)
  const std::string &href() const;
  void setHref(const std::string &href);

  void toXml(SvgRope &output) const override;
  bool setAttribute(const std::string &key, const std::string &value) override;
  bool attributeXml(SvgRope &output) const override;
};
