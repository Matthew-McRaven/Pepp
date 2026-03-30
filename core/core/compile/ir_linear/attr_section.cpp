#include "attr_section.hpp"
#include "fmt/format.h"

int pepp::tc::SectionFlags::type() const { return TYPE; }

pepp::tc::SectionFlags::SectionFlags(bool r, bool w, bool x, bool z) : r(r), w(w), x(x), z(z) {}

bool pepp::tc::SectionFlags::operator==(const SectionFlags &rhs) const {
  return r == rhs.r && w == rhs.w && x == rhs.x && z == rhs.z;
}

std::string pepp::tc::SectionFlags::to_string() const {
  static const std::string rs = "r", ws = "w", xs = "x", zs = "z", e = "";
  return fmt::format("{}{}{}{}", r ? rs : e, w ? ws : e, x ? xs : e, z ? zs : e);
}
