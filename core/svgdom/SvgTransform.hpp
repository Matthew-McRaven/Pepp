#pragma once

#include <expected>
#include <fmt/format.h> //  For fmt::format
#include <list>
#include <string>
using namespace std::string_literals;

/*  SvgTransform is a "polymorphic" type based on Javascript standards. It's
 *  transform type can change based on updates to it's properties. This mimics
 *  the behavior of SVGTransform in Javascript. For example, if type is Scale,
 *  but SkewX is set, type changes to SkewX automatically.
 *
 *  See https://developer.mozilla.org/en-US/docs/Web/API/SVGTransform.
 */
class SvgTransform {
public:
  //  Order of transformation matters
  //  Apply tranformation in order that they are entered
  enum class TransformType { Unknown = 0, Matrix, Rotate, Scale, SkewX, SkewY, Translate };

private:
  //  Note, all dimensions are typeless. They pick up parent units.
  double _arg0{}, _arg1{}, _arg2{};

  TransformType _type = TransformType::Unknown;
  std::string _unparsed;

public:
  SvgTransform() = default;
  ~SvgTransform() = default;
  SvgTransform(const SvgTransform &) = default;
  SvgTransform &operator=(const SvgTransform &) = default;
  SvgTransform(SvgTransform &&) noexcept = default;
  SvgTransform &operator=(SvgTransform &&) noexcept = default;

  TransformType type() const noexcept;
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
  // bool fromString(const std::string &value);
  const std::string toString() const;
};

// Based on Javascript SVGTransformList
// See https://developer.mozilla.org/en-US/docs/Web/API/SVGTransformList
class SvgTransformList {
  std::list<SvgTransform> _transforms;
  double _a{1.0}, _b{}, _c{}, _d{1.0}, _e{}, _f{}; //  Matrix values

  double _rotateAngle = 0;
  double _rotateX = 0;
  double _rotateY = 0;
  double _scaleX = 1;
  double _scaleY = 1;
  double _skewX = 0;
  double _skewY = 0;
  double _translateX = 0;
  double _translateY = 0;

  void createMatrix();
  void multiply(const double ma, const double mb, const double mc, const double md, const double me, const double mf);
  void composeSequentialMatrix();

public:
  SvgTransformList() = default;
  ~SvgTransformList() = default;
  SvgTransformList(const SvgTransformList &) = default;
  SvgTransformList &operator=(const SvgTransformList &) = default;
  SvgTransformList(SvgTransformList &&) noexcept = default;
  SvgTransformList &operator=(SvgTransformList &&) noexcept = default;

  void clear();
  size_t size() const noexcept;
  auto getItem(const size_t pos) -> std::expected<SvgTransform, bool>;
  // const auto &getItem(size_t pos) const -> std::expected<const SvgTransform&, bool>;
  void insertItemBefore();
  void replaceItem();
  SvgTransform &appendItem();
  void createSvgTransformFromMatrix();
  void consolidate();

  //  Persistence functions
  bool fromString(const std::string &value);
  const std::string toString() const;
};
