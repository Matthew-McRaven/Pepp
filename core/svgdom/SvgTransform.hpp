#pragma once

#include <fmt/format.h> //  For fmt::format
#include <list>
#include <string>
using namespace std::string_literals;

class SvgTransform {

  //  Order of transformation matters
  //  Apply tranformation in order that they are entered
  enum class TransformType { None = 0, Rotate, Scale, SkewX, SkewY, Translate };

  //  Note, all dimensions are typeless. They pick up parent units.
  double _rotateAngle = 0;
  double _rotateX = 0;
  double _rotateY = 0;
  double _scaleX = 1;
  double _scaleY = 0; //  Use scaleX if 0
  double _skewX = 0;
  double _skewY = 0;
  double _translateX = 0;
  double _translateY = 0;

  std::list<TransformType> _order;

public:
  SvgTransform() = default;
  ~SvgTransform() = default;
  SvgTransform(const SvgTransform &) = default;
  SvgTransform &operator=(const SvgTransform &) = default;
  SvgTransform(SvgTransform &&) noexcept = default;
  SvgTransform &operator=(SvgTransform &&) noexcept = default;

  double rotateAngle() const noexcept;
  double rotateX() const noexcept;
  double rotateY() const noexcept;
  double scaleX() const noexcept;
  double scaleY() const noexcept;
  double skewX() const noexcept;
  double skewY() const noexcept;
  double translateX() const noexcept;
  double translateY() const noexcept;

  void setRotate(const double angle, const double cx = 0, const double cy = 0);
  //  Negative scale flips image
  void setScale(const double sx, const double sy = 0);
  void setSkewX(const double angle);
  void setSkewY(const double angle);
  void setTranslate(const double tx, const double ty = 0);

  void clear();
  bool fromString(const std::string &value);
  const std::string toString() const;
};
