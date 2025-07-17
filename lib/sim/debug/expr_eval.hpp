#pragma once
#include <QtCore>
#include "expr_cache.hpp"
#include "expr_value.hpp"

namespace pepp::debug {
// Controls how previously computed values will be used for future evaluations.
enum class CachePolicy {
  // Use cache if it is set, ignoring dirtiness and (recursive) voltatility.
  UseDirtyAlways,
  // Use cache if the term is not dirty, ignoring (recursive) voltatility.
  UseAlways,
  // Use cache if the term is not dirty and not (recursively) volatile.
  UseNonVolatiles,
  // Ignore any cached value -- recompute the entire tree.
  UseNever,
};

// Variables, registers are volatile.
// All volatile "things" must be updated manually at the end of each simulator step
// Then you can evaluate your top level expressions and re-generate caches if values changed.
// This prevents having to recursively evaluate the entire tree to check for a volatile change.
struct CVQualifiers {
  enum { Constant = 0x1, Volatile = 0x2 };
};

// Combined above definitions to cache an optional value with versioning.
struct OptVal {
  OptVal() = default;
  explicit OptVal(Value value) : value(value) {}
  OptVal(const OptVal &other) = default;
  OptVal &operator=(const OptVal &other) = default;
  OptVal(OptVal &&other) = default;
  OptVal &operator=(OptVal &&other) = default;
  std::optional<Value> value = std::nullopt;
};
using EvaluationCache = Cached<Versioned<OptVal>>;

struct Environment {
  virtual types::TypeInfo *type_info() = 0;
  virtual types::TypeInfo const *type_info() const = 0;
  // Read some bytes of memory (modulo the size of the address space) in the platform's preferred endianness
  virtual uint8_t read_mem_u8(uint32_t address) const = 0;
  virtual uint16_t read_mem_u16(uint32_t address) const = 0;
  virtual Value evaluate_variable(QStringView name) const = 0;
  virtual uint32_t cache_debug_variable_name(QStringView name) const = 0;
  virtual Value evaluate_debug_variable(uint32_t cache_index) const = 0;
};
struct ZeroEnvironment : public Environment {
  inline types::TypeInfo *type_info() override { return &_typeInfo; }
  inline types::TypeInfo const *type_info() const override { return &_typeInfo; }
  inline uint8_t read_mem_u8(uint32_t address) const override { return 0; }
  inline uint16_t read_mem_u16(uint32_t address) const override { return 0; }
  inline Value evaluate_variable(QStringView name) const override { return VPrimitive::from_int(int16_t(0)); };
  inline uint32_t cache_debug_variable_name(QStringView name) const override { return 0; }
  inline Value evaluate_debug_variable(uint32_t cache_index) const override {
    return VPrimitive::from_int(int16_t(0));
  };

protected:
  types::TypeInfo _typeInfo;
};

class Term;
// Helper which caches the last (locally) evaluated value for a term.
// Can check how outdated the value is by comparing EvaluationCache::version
struct CachedEvaluator {
  CachedEvaluator() = default;
  explicit CachedEvaluator(std::shared_ptr<Term> term);
  CachedEvaluator(const CachedEvaluator &) = default;
  CachedEvaluator &operator=(const CachedEvaluator &) = default;
  CachedEvaluator(CachedEvaluator &&) = default;
  CachedEvaluator &operator=(CachedEvaluator &&) = default;

  // True when term.cache().version != _cache.version
  bool dirty();
  EvaluationCache cache() const;
  Value evaluate(CachePolicy mode, Environment &env);
  std::shared_ptr<Term> term();

private:
  EvaluationCache _cache = {};
  std::shared_ptr<Term> _term = nullptr;
};
} // namespace pepp::debug

