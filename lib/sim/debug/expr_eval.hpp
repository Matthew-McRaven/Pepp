#pragma once
#include <QtCore>
#include "expr_types.hpp"
namespace pepp::debug {

enum class EvaluationMode { UseCache, RecomputeSelf, RecomputeTree };
EvaluationMode mode_for_child(EvaluationMode current);

// Variables, registers are volatile.
// All volatile "things" must be updated manually at the end of each simulator step
// Then you can evaluate your top level expressions and re-generate caches if values changed.
// This prevents having to recursively evaluate the entire tree to check for a volatile change.
struct CVQualifiers {
  enum { Constant = 0x1, Volatile = 0x2 };
};

struct EvaluationCache {
  bool dirty = false;
  std::optional<TypedBits> value = std::nullopt;
};
} // namespace pepp::debug

