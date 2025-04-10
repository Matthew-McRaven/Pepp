#pragma once
#include <QtCore>
#include "expr_types.hpp"
namespace pepp::debug {

// Controls how previously computed & cached values should be used for future evaluations.
enum class CachePolicy {
  // If the cache is not dirty, uncoditionally return it. This applies even if this term or one of its dependencies is
  // volatile.
  UseAlways,
  // If the cache is not dirty and neither this term nor its dependencies are volatile, use the cache.
  // If either this term or its dependencies are volatile, we will recursively re-evaluate the volatile portions and use
  // non-dirty cache values for non-volatile dependencies.
  UseNonVolatiles,
  // Ignore the cache, recompute the entire tree from scratch and rebuild caches
  UseNever,
};

// Variables, registers are volatile.
// All volatile "things" must be updated manually at the end of each simulator step
// Then you can evaluate your top level expressions and re-generate caches if values changed.
// This prevents having to recursively evaluate the entire tree to check for a volatile change.
struct CVQualifiers {
  enum { Constant = 0x1, Volatile = 0x2 };
};

struct EvaluationCache {
  bool dirty = false, depends_on_volatiles = false;
  std::optional<TypedBits> value = std::nullopt;
};

struct Environment {
  // Read some bytes of memory (modulo the size of the address space) in the platform's preferred endianness
  std::function<uint8_t(uint32_t)> read_mem_u8 = [](uint32_t) { return 0; };
  std::function<uint16_t(uint32_t)> read_mem_u16 = [](uint32_t) { return 0; };
};
} // namespace pepp::debug

