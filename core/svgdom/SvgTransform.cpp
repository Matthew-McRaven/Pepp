#include "SvgTransform.hpp"

#include <cmath>
#include <ranges>
#include <string_view>
#include <vector>

using namespace std::string_literals;

// #include "core/langs/expr/parser.hpp" //  Pepp parser
#include "SvgParser.hpp"

SvgTransform::TransformType SvgTransform::type() const noexcept { return _type; }

double SvgTransform::rotateAngle() const noexcept {
  if (_type != TransformType::Rotate) return 0.0;
  return _arg0;
}
double SvgTransform::rotateX() const noexcept {
  if (_type != TransformType::Rotate) return 0.0;
  return _arg1;
}
double SvgTransform::rotateY() const noexcept {
  if (_type != TransformType::Rotate) return 0.0;
  return _arg2;
}
double SvgTransform::scaleX() const noexcept {
  if (_type != TransformType::Scale) return 1.0;
  return _arg0;
}
double SvgTransform::scaleY() const noexcept {
  if (_type != TransformType::Scale) return 1.0;
  return _arg1;
}
double SvgTransform::skewX() const noexcept {
  if (_type != TransformType::SkewX) return 0.0;
  return _arg0;
}
double SvgTransform::skewY() const noexcept {
  if (_type != TransformType::SkewY) return 0.0;
  return _arg0;
}
double SvgTransform::translateX() const noexcept {
  if (_type != TransformType::Translate) return 0.0;
  return _arg0;
}
double SvgTransform::translateY() const noexcept {
  if (_type != TransformType::Translate) return 0.0;
  return _arg1;
}

void SvgTransform::setRotate(const double angle, const double cx, const double cy) {
  _type = TransformType::Rotate;
  _arg0 = fmod(angle, 360.0); //  Limit to 360 degrees
  _arg1 = cx;
  _arg2 = cy;
}

void SvgTransform::setScale(const double sx, const double sy) {
  _type = TransformType::Scale;
  _arg0 = sx;
  _arg1 = sy;
}

void SvgTransform::setSkewX(const double angle) {
  _type = TransformType::SkewX;
  _arg0 = fmod(angle, 360.0); //  Limit to 360 degrees
}

void SvgTransform::setSkewY(const double angle) {
  _type = TransformType::SkewY;
  _arg0 = fmod(angle, 360.0); //  Limit to 360 degrees
}

void SvgTransform::setTranslate(const double tx, const double ty) {
  _type = TransformType::Translate;
  _arg0 = tx;
  _arg1 = ty;
}

void SvgTransform::clear() {
  _arg0 = _arg1 = _arg2 = 0.0;
  _type = TransformType::Unknown;
  _unparsed.clear();
}

/*bool SvgTransform::fromString(const std::string &value) {
  // using namespace pepp::tc::expr;
  // const auto parse_result = parse(value);
  // if (!std::holds_alternative<Parsed>(parse_result)) {
  //   There was a parsing issue
  // return false;
  //}
  // const auto &result = std::get<Parsed>(parse_result);

  std::string_view view{value};
  const auto results = FunctionParser::parse(view);

  //  Parsing error
  if (!results.has_value()) return false;

  for (const auto &result : *results) {
    const auto name = result.first;
    auto &args = result.second;
    if (name.starts_with("rotate"s)) {
      assert(args.size() > 0 && args.size() < 4);
      switch (args.size()) {
      case 0: continue;
      case 1: setRotate(args[0]); continue;
      case 2: setRotate(args[0], args[1]); continue;
      default: setRotate(args[0], args[1], args[2]); continue;
      }

    } else if (name.starts_with("scale")) {
      assert(args.size() > 0 && args.size() < 3);
      switch (args.size()) {
      case 0: continue;
      case 1: setScale(args[0]); continue;
      default: setScale(args[0], args[1]); continue;
      }
    } else if (name.starts_with("skewX")) {
      assert(args.size() == 1);
      switch (args.size()) {
      case 0: continue;
      default: setSkewX(args[0]); continue;
      }
    } else if (name.starts_with("skewY")) {
      assert(args.size() == 1);
      switch (args.size()) {
      case 0: continue;
      default: setSkewY(args[0]); continue;
      }
    } else if (name.starts_with("translate")) {
      assert(args.size() > 0 && args.size() < 3);
      switch (args.size()) {
      case 0: continue;
      case 1: setTranslate(args[0]); continue;
      default: setTranslate(args[0], args[1]); continue;
      }
    }
  }
  return true;
}*/

const std::string SvgTransform::toString() const {

  std::string buffer;
  //  Translate to string based on current type.
  //  Note type can change several times during its lifetime.
  //  Use last type for persistence.
  switch (_type) {
  case TransformType::Rotate:
    //  Must have a rotation to output string
    if (_arg0 != 0) {
      if (_arg1 != 0.0 || _arg2 != 0.0) {
        //  Rotation was set. Either X or Y is valid
        buffer += fmt::format("rotate({} {} {})", _arg0, _arg1, _arg2);
      } else {
        buffer += fmt::format("rotate({})", _arg0);
      }
    }
    break;
  case TransformType::Scale:
    if (_arg0 != 1) {
      if (_arg1 != 0) {
        buffer += fmt::format("scale({} {})", _arg0, _arg1);
      } else {
        buffer += fmt::format("scale({})", _arg0);
      }
    }
    break;
  case TransformType::SkewX:
    if (_arg0 != 0) {
      buffer += fmt::format("skewX({})", _arg0);
    }
    break;
  case TransformType::SkewY:
    if (_arg0 != 0) {
      buffer += fmt::format("skewY({})", _arg0);
    }
    break;

  case TransformType::Translate:
    if (_arg0 != 0) {
      if (_arg1 != 0) {
        buffer += fmt::format("translate({} {})", _arg0, _arg1);
      } else {
        buffer += fmt::format("translate({})", _arg0);
      }
    }
    break;
  default: buffer = _unparsed;
  } //  switch(_type)

  return std::move(buffer);
}

//  SvgTransformList

SvgTransform &SvgTransformList::appendItem() { return _transforms.emplace_back(); }

void SvgTransformList::clear() { _transforms.clear(); }

size_t SvgTransformList::size() const noexcept { return _transforms.size(); }

SvgTransform *SvgTransformList::getItem(const size_t pos) {
  if (pos > _transforms.size()) return nullptr;

  auto it = std::next(_transforms.begin(), pos);
  if (it == _transforms.end()) return nullptr;

  return &(*it);
}

bool SvgTransformList::removeItem(const size_t pos) {
  if (pos > _transforms.size()) return false;

  auto it = std::next(_transforms.begin(), pos);
  if (it == _transforms.end()) return false;

  _transforms.erase(it);
  return true;
}

bool SvgTransformList::fromString(const std::string &value) {
  // using namespace pepp::tc::expr;
  // const auto parse_result = parse(value);
  // if (!std::holds_alternative<Parsed>(parse_result)) {
  //   There was a parsing issue
  // return false;
  //}
  // const auto &result = std::get<Parsed>(parse_result);

  std::string_view view{value};
  const auto results = FunctionParser::parse(view);

  //  Parsing error
  if (!results.has_value()) return false;

  for (const auto &result : *results) {
    const auto name = result.first;
    auto &args = result.second;
    if (args.empty())
      //  If there are no args, then do not update transform.
      continue;
    auto &transform = appendItem();
    if (name.starts_with("rotate"s)) {
      assert(args.size() < 4);
      switch (args.size()) {
      case 1: transform.setRotate(args[0]); continue;
      case 2: transform.setRotate(args[0], args[1]); continue;
      default: transform.setRotate(args[0], args[1], args[2]); continue;
      }

    } else if (name.starts_with("scale")) {
      assert(args.size() < 3);
      switch (args.size()) {
      case 1: transform.setScale(args[0]); continue;
      default: transform.setScale(args[0], args[1]); continue;
      }
    } else if (name.starts_with("skewX")) {
      assert(args.size() == 1);
      transform.setSkewX(args[0]);
      continue;

    } else if (name.starts_with("skewY")) {
      assert(args.size() == 1);
      transform.setSkewY(args[0]);
      continue;

    } else if (name.starts_with("translate")) {
      assert(args.size() < 3);
      switch (args.size()) {
      case 1: transform.setTranslate(args[0]); continue;
      default: transform.setTranslate(args[0], args[1]); continue;
      }
    }
  }
  return true;
}

const std::string SvgTransformList::toString() const {
  static std::string header = "transform=\""s;
  std::string buffer;

  //  If multiple arguments, separate with a space
  for (const auto &transform : _transforms) {
    if (buffer.size() > 0 && !buffer.ends_with(' ')) {
      buffer += " "s;
    }

    buffer += transform.toString();
  }

  //  If buffer is completely empty, there is no transform string
  if (buffer.empty()) return ""s;

  return std::move(header + buffer + "\"");
}

//  Matrix operations
//  Recalculate transforms from matrix
void SvgTransformList::consolidate() {
  //  First, translate current tranformations into matrix
  composeSequentialMatrix();

  //  Reset transform list
  _transforms.clear();

  // Translate matrix into tranformations
  // 1. Translation is direct
  if (!isNearlyZero(_e) || !isNearlyZero(_f)) appendItem().setTranslate(_e, _f);

  // 2. Calculate Scale factors (magnitudes of the column vectors)
  // If the determinant is negative, correct the scale and flip the sign
  const double determinant = _a * _d - _b * _c;

  const double scaleX = std::sqrt(_a * _a + _b * _b);
  const double scaleY = std::sqrt(_c * _c + _d * _d);

  //  If both values are 1, then there is no transform
  if (!isNearlyZero(scaleX - 1) || !isNearlyZero(scaleY - 1)) {
    if (!isNearlyZero(scaleY) || !isNearlyZero(scaleY - scaleX))
      //  X and Y directions have scale
      appendItem().setScale(scaleX, determinant < 0 ? scaleY : -scaleY);
    else
      //  If scaleY is zero, use scaleX value
      appendItem().setScale(scaleX);
  }

  // Rotation is isolated from the first column vector [a, b]
  const double rotateAngle = std::atan2(_b, _a) * SvgTransformList::degreesToRadians;
  if (!isNearlyZero(rotateAngle))
    //  There is a non-zero rotation
    appendItem().setRotate(rotateAngle);

  // Skew X is the remaining angular difference between the two basis vectors
  // We use the dot product and determinant values to extract it cleanly
  double skewX = 0;
  if (!isNearlyZero(_a) || !isNearlyZero(_b)) {
    skewX = std::atan2(_a * _c + _b * _d, determinant) * SvgTransformList::degreesToRadians;
  } else {
    skewX = std::atan2(_c, _d) * SvgTransformList::degreesToRadians;
  }

  if (!isNearlyZero(skewX))
    //  There is a non-zero skew in X direction
    appendItem().setSkewX(skewX);

  // In this standard model, skewY is fully absorbed by rotation and skewX
  // In 2D matrix transformations, a combination of Rotation + Skew X is
  // mathematically identical to a combination of Skew X + Skew Y.
}

//  Only works for single function. does not work with 2 "translate" function
//  (or any other duplicates like skewY skewY).
/*void SvgTransformList::createMatrix() {
  //  Convert all angles from degrees to radians
  const double rRad = _rotateAngle * SvgTransformList::radiansToDegrees;
  const double skXRad = _skewX * SvgTransformList::radiansToDegrees;
  const double skYRad = _skewY * SvgTransformList::radiansToDegrees;

  // Default scales to 1 if not provided
  // const double sx = scale &&scale.x != = undefined ? scale.x : 1;
  // const double sy = scale &&scale.y != = undefined ? scale.y : 1;

  // Default translations to 0 if not provided
  // const double tx = translate &&translate.x != = undefined ? translate.x : 0;
  // const double ty = translate &&translate.y != = undefined ? translate.y : 0;

  // 1. Calculate individual trigonometric values
  const double cosR = std::cos(rRad);
  const double sinR = std::sin(rRad);
  const double tanX = std::tan(skXRad);
  const double tanY = std::tan(skYRad);

  // 2. Pre-multiply combinations step-by-step to arrive at the final 6 coefficients
  // This computes: Translation * Rotation * SkewY * SkewX * Scale
  _a = _scaleX * (cosR * (1 + tanX * tanY) + sinR * tanY);
  _b = _scaleX * (sinR * (1 + tanX * tanY) - cosR * tanY);
  _c = _scaleY * (cosR * tanX - sinR);
  _d = _scaleY * (sinR * tanX + cosR);
  _e = _translateX + _rotateX - (_a * _rotateX + _c * _rotateY);
  _f = _translateY + _rotateY - (_b * _rotateX + _d * _rotateY);
}*/

void SvgTransformList::resetMatrix() {
  // Start with an Identity Matrix (no changes)
  _a = 1, _b = 0, _c = 0, _d = 1, _e = 0, _f = 0;
}

// Helper helper function to multiply current state by an incoming transformation step
// [a c e]   [ma mc me]
// [b d f] * [mb md mf]
// [0 0 1]   [0  0  1 ]
void SvgTransformList::multiply(const double ma, const double mb, const double mc, const double md, const double me,
                                const double mf) {
  const double nextA = _a * ma + _c * mb;
  const double nextB = _b * ma + _d * mb;
  const double nextC = _a * mc + _c * md;
  const double nextD = _b * mc + _d * md;
  const double nextE = _a * me + _c * mf + _e;
  const double nextF = _b * me + _d * mf + _f;

  _a = nextA;
  _b = nextB;
  _c = nextC;
  _d = nextD;
  _e = nextE;
  _f = nextF;
}

void SvgTransformList::composeSequentialMatrix() {
  //  Convert all angles from degrees to radians

  for (const auto &transform : _transforms) {
    switch (transform.type()) {
    case SvgTransform::TransformType::Rotate:
      setRotate(transform.rotateAngle(), transform.rotateX(), transform.rotateY());
      break;
    case SvgTransform::TransformType::Scale: setScale(transform.scaleX(), transform.scaleY()); break;
    case SvgTransform::TransformType::SkewX: setSkewX(transform.skewX()); break;
    case SvgTransform::TransformType::SkewY: setSkewY(transform.skewY()); break;
    case SvgTransform::TransformType::Translate: setTranslate(transform.translateX(), transform.translateY()); break;
    default:
      //  Other tranformations types are not output
      break;
    }
  }
}

void SvgTransformList::setRotate(const double angle, const double cx, const double cy) {
  //  If no rotation, just return.
  //  If rotation is zero, this is a non-operation
  if (isNearlyZero(angle)) return;

  bool hasOriginPivot = false;
  // STEP 1: Handle transform-origin pivot (Move origin to 0,0)
  if (!isNearlyZero(cx) || !isNearlyZero(cy)) {
    multiply(1, 0, 0, 1, -cx, -cy);
    hasOriginPivot = true;
  }

  // STEP 4: Rotate
  const double cosR = std::cos(angle);
  const double sinR = std::sin(angle);
  multiply(cosR, sinR, -sinR, cosR, 0, 0);

  // STEP 7: Reset transform-origin pivot (Move origin back)
  if (hasOriginPivot) {
    multiply(1, 0, 0, 1, cx, cy);
  }
}

void SvgTransformList::setScale(const double sx, const double sy) {
  const double newSy = (std::fabs(sy) < SvgTransformList::epsilon) ? sx : sy;
  // STEP 3: Scale
  if (!isNearlyZero(sx - 1) || !isNearlyZero(newSy - 1)) {
    multiply(sx, 0, 0, newSy, 0, 0);
  }
}

void SvgTransformList::setSkewX(const double angle) {
  // STEP 5: Skew X
  if (!isNearlyZero(angle)) {
    //  convert to radians. Input is degrees
    multiply(1, 0, std::tan(angle * SvgTransformList::radiansToDegrees), 1, 0, 0);
  }
}

void SvgTransformList::setSkewY(const double angle) {
  // STEP 6: Skew Y
  if (!isNearlyZero(angle)) {
    multiply(1, std::tan(angle * SvgTransformList::radiansToDegrees), 0, 1, 0, 0);
  }
}

void SvgTransformList::setTranslate(const double tx, const double ty) {
  // STEP 2: Translate
  //  If both values are zero, this is a non-operation
  if (!isNearlyZero(tx) || !isNearlyZero(ty)) {
    multiply(1, 0, 0, 1, tx, ty);
  }
}

bool SvgTransformList::isNearlyZero(const double x) { return std::fabs(x) < SvgTransformList::epsilon; }