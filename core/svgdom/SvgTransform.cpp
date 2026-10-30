#include "SvgTransform.hpp"

#include <cmath>
#include <numbers>
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

auto SvgTransformList::getItem(const size_t pos) -> std::expected<SvgTransform, bool> {
  if (pos > _transforms.size()) return std::unexpected(false);

  auto it = std::next(_transforms.begin(), pos);
  if (it == _transforms.end()) return std::unexpected(false);

  return *it;
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
void SvgTransformList::consolidate() {
  // 1. Translation is direct
  _translateX = _e;
  _translateY = _f;

  // 2. Calculate Scale factors (magnitudes of the column vectors)
  _scaleX = std::sqrt(_a * _a + _b * _b);
  _scaleY = std::sqrt(_c * _c + _d * _d);

  // 3. Calculate Rotation and Skew
  // Determine if the matrix has a reflection (flipped)
  const double determinant = _a * _d - _b * _c;
  if (determinant < 0) {
    // If the determinant is negative, correct the scale and flip the sign
    _scaleY = -_scaleY;
  }

  //  Convert all angles from degrees to radians
  //  Temporary used in next 3 formulas
  const double degreesToRadians = 180.0 / std::numbers::inv_pi;

  // Rotation is isolated from the first column vector [a, b]
  _rotateAngle = std::atan2(_b, _a) * degreesToRadians;

  // Skew X is the remaining angular difference between the two basis vectors
  // We use the dot product and determinant values to extract it cleanly
  _skewX = 0;
  if (_a != 0 || _b != 0) {
    _skewX = std::atan2(_a * _c + _b * _d, determinant) * degreesToRadians;
  } else {
    _skewX = std::atan2(_c, _d) * degreesToRadians;
  }
  // In this standard model, skewY is fully absorbed by rotation and skewX
  // In 2D matrix transformations, a combination of Rotation + Skew X is
  // mathematically identical to a combination of Skew X + Skew Y.
  _skewY = 0;
}

//  Only works for single function. does not work with 2 "translate" function
//  (or any other duplicates like skewY skewY).
void SvgTransformList::createMatrix() {
  //  Convert all angles from degrees to radians
  //  Temporary used in next 3 formulas
  const double radiansToDegrees = std::numbers::inv_pi / 180.0;

  const double rRad = _rotateAngle * radiansToDegrees;
  const double skXRad = _skewX * radiansToDegrees;
  const double skYRad = _skewY * radiansToDegrees;

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
  //  Temporary used in next 3 formulas
  const double radiansToDegrees = std::numbers::inv_pi / 180.0;

  // 1. Defaults with nullish coalescing
  // const tx = translate?.x ?? 0;
  // const ty = translate?.y ?? 0;
  // const sx = scale?.x ?? 1;
  // const sy = scale?.y ?? 1;
  const double rRad = _rotateAngle * radiansToDegrees;
  const double skXRad = _skewX * radiansToDegrees;
  const double skYRad = _skewY * radiansToDegrees;
  // const double cx = origin?.x ?? 0;
  // const double cy = origin?.y ?? 0;

  // Start with an Identity Matrix (no changes)
  _a = 1, _b = 0, _c = 0, _d = 1, _e = 0, _f = 0;

  // STEP 1: Handle transform-origin pivot (Move origin to 0,0)
  if (_rotateX != 0 || _rotateY != 0) {
    multiply(1, 0, 0, 1, -_rotateX, -_rotateY);
  }

  // STEP 2: Translate
  if (_translateX != 0 || _translateY != 0) {
    multiply(1, 0, 0, 1, _translateX, _translateY);
  }

  // STEP 3: Scale
  if (_scaleX != 1 || _scaleY != 1) {
    multiply(_scaleX, 0, 0, _scaleY, 0, 0);
  }

  // STEP 4: Rotate
  if (rRad != 0) {
    const double cosR = std::cos(rRad);
    const double sinR = std::sin(rRad);
    multiply(cosR, sinR, -sinR, cosR, 0, 0);
  }

  // STEP 5: Skew X
  if (skXRad != 0) {
    multiply(1, 0, std::tan(skXRad), 1, 0, 0);
  }

  // STEP 6: Skew Y
  if (skYRad != 0) {
    multiply(1, std::tan(skYRad), 0, 1, 0, 0);
  }

  // STEP 7: Reset transform-origin pivot (Move origin back)
  if (_rotateX != 0 || _rotateY != 0) {
    multiply(1, 0, 0, 1, _rotateX, _rotateY);
  }
}