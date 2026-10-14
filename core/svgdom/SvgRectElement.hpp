#pragma once

#include <string>

#include "SvgElement.hpp"
#include "utility_p.hpp"

class SvgRectElement final : public Cloneable<SvgRectElement, SvgElement>
{
  SvgUnitValue _rx;
  SvgUnitValue _ry;

  const std::string &value() const override;
  void setValue(const std::string &value) override;

public:
  static constexpr SvgType::Type TYPE = SvgType::Type::SvgRectElement;

  SvgRectElement();
  explicit SvgRectElement(const std::string &name);
  ~SvgRectElement() = default;
  // Allo copy and move
  SvgRectElement(const SvgRectElement &) = default;
  SvgRectElement &operator=(const SvgRectElement &) = default;
  SvgRectElement(SvgRectElement &&) noexcept = default;
  SvgRectElement &operator=(SvgRectElement &&) noexcept = default;

  //	User access functions
  SvgType::Type elementType() const override;

  //  Values can be changed, but not units of measure (yet)
  SvgUnitValue rx() const;
  void setRx(double x = 0);
  void setRx(const std::string_view sv);
  SvgUnitValue ry() const;
  void setRy(double y = 0);
  void setRy(const std::string_view sv);

  void toXml(SvgRope &output) const override;
  bool setAttribute(const std::string &key, const std::string &value) override;
  bool attributeXml(SvgRope &output) const override;
};
