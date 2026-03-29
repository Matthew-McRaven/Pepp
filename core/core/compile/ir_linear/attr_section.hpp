#pragma once

#include <string>
#include "core/compile/ir_linear/attr_base.hpp"

namespace pepp::tc {
struct SectionFlags : public AAttribute {
  static constexpr int TYPE = static_cast<int>(Type::SectionFlags);
  int type() const override;
  SectionFlags(bool r, bool w, bool x, bool z);
  // Must update == if flags changes. Cannot use default due to abstract base class.
  bool r = false, w = false, x = false, z = false;
  bool operator==(const SectionFlags &rhs) const;
  std::string to_string() const;
};
} // namespace pepp::tc
