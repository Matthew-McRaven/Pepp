#include "SvgTransform.hpp"

#include <cmath>

double SvgTransform::rotateAngle() const noexcept { return _rotateAngle; }
double SvgTransform::rotateX() const noexcept { return _rotateX; }
double SvgTransform::rotateY() const noexcept { return _rotateY; }
double SvgTransform::scaleX() const noexcept { return _scaleX; }
double SvgTransform::scaleY() const noexcept { return _scaleY; }
double SvgTransform::skewX() const noexcept { return _skewX; }
double SvgTransform::skewY() const noexcept { return _skewY; }
double SvgTransform::translateX() const noexcept { return _translateX; }
double SvgTransform::translateY() const noexcept { return _translateY; }

void SvgTransform::setRotate(const double angle, const double cx, const double cy) {
  _rotateAngle = fmod(angle, 360.0); //  Limit to 360 degrees
  _rotateX = cx;
  _rotateY = cy;

  //  Note, setting attribute twice will move it to the end
  _order.remove(TransformType::Rotate);
  //  If reset to zero, remove from queue and do not output
  if (_rotateAngle != 0) _order.push_back(TransformType::Rotate);
}

void SvgTransform::setScale(const double sx, const double sy) {
  _scaleX = sx;
  _scaleY = sy;

  //  Note, setting attribute twice will move it to the end
  _order.remove(TransformType::Scale);
  //  If reset to default values, remove from queue and do not output
  if (_scaleX != 1 || _scaleY != 0) _order.push_back(TransformType::Scale);
}

void SvgTransform::setSkewX(const double angle) {
  _skewX = fmod(angle, 360.0); //  Limit to 360 degrees

  //  Note, setting attribute twice will move it to the end
  _order.remove(TransformType::SkewX);
  //  If reset to default values, remove from queue and do not output
  if (_skewX != 0) _order.push_back(TransformType::SkewX);
}

void SvgTransform::setSkewY(const double angle) {
  _skewY = fmod(angle, 360.0); //  Limit to 360 degrees

  //  Note, setting attribute twice will move it to the end
  _order.remove(TransformType::SkewY);
  //  If reset to default values, remove from queue and do not output
  if (_skewY != 0) _order.push_back(TransformType::SkewY);
}

void SvgTransform::setTranslate(const double tx, const double ty) {
  _translateX = tx;
  _translateY = ty;

  //  Note, setting attribute twice will move it to the end
  _order.remove(TransformType::Translate);
  //  If reset to default values, remove from queue and do not output
  if (_translateX != 0 || _translateY != 0) _order.push_back(TransformType::Translate);
}

void SvgTransform::clear() {
  _rotateAngle = 0.0;
  _rotateX = 0.0;
  _rotateY = 0.0;
  _scaleX = 1.0;
  _scaleY = 0.0; //  Use scaleX if 0
  _skewX = 0.0;
  _skewY = 0.0;
  _translateX = 0.0;
  _translateY = 0.0;

  _order.clear();
}

const std::string SvgTransform::toString() const {
  bool first = true;
  std::string buffer;
  for (const auto type : _order) {
    if (first) {
      buffer = "transform=\""s;
      first = false;
    } else buffer += " "s;

    switch (type) {
    case TransformType::Rotate:
      //  Must have a rotation to output string
      if (_rotateAngle != 0) {
        if (_rotateX != 0.0 || _rotateY != 0.0) {
          //  Rotation was set. Either X or Y is valid
          buffer += fmt::format("rotate({} {} {})", _rotateAngle, _rotateX, _rotateY);
        } else {
          buffer += fmt::format("rotate({})", _rotateAngle);
        }
      } else {
        if (_order.size() == 1) {
          //  Missing rotation as only input
          //  should not generate output
          buffer.clear();
        }
      }
      continue;
    case TransformType::Scale:
      if (_scaleX != 1) {
        if (_scaleY != 0) {
          buffer += fmt::format("scale({} {})", _scaleX, _scaleY);
        } else {
          buffer += fmt::format("scale({})", _scaleX);
        }
      }
      continue;
    case TransformType::SkewX:
      if (_skewX != 0) {
        buffer += fmt::format("skewX({})", _skewX);
      }
      continue;
    case TransformType::SkewY:
      if (_skewY != 0) {
        buffer += fmt::format("skewY({})", _skewY);
      }
      continue;

    case TransformType::Translate:
      if (_translateX != 0) {
        if (_translateY != 0) {
          buffer += fmt::format("translate({} {})", _translateX, _translateY);
        } else {
          buffer += fmt::format("translate({})", _translateX);
        }
      }
      continue;
    default: continue;
    }
  } //  for (auto type : _order)
  if (!buffer.empty()) buffer += "\""s;
  return std::move(buffer);
}