#pragma once
#include <QtCore>
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

// Split versioning logic from storing the value using CRTP so that the same classes can be used for values and types
template <typename T> struct Versioned : public T {
  uint32_t version = 0;
  // Used so that Versioned<OptVal>(Value) works without any explicit casts
  template <typename U> explicit Versioned(U value) : T(value), version(0) {}
  Versioned() = default;
  Versioned(const Versioned &other) = default;
  Versioned &operator=(const Versioned &other) = default;
  Versioned(Versioned &&other) = default;
  Versioned &operator=(Versioned &&other) = default;
};

template <typename T> struct Cached : public T {
  template <typename U> explicit Cached(U value) : T(value), _dirty(false), _depends_on_volatiles(false) {}
  Cached() = default;
  Cached(const Cached &other) = default;
  Cached &operator=(const Cached &other) = default;
  Cached(Cached &&other) = default;
  Cached &operator=(Cached &&other) = default;

  bool dirty() const { return _dirty; }
  // Increment version on dirty and not clean, which means we should have fewer places where we need to touch version.
  void mark_dirty() { _dirty = true; }
  void mark_clean(bool increment_version = true) {
    _dirty = false;
    if constexpr (requires { this->version++; }) {
      if (increment_version) this->version++;
    }
  }
  bool depends_on_volatiles() const { return _depends_on_volatiles; }
  void set_depends_on_volatiles(bool depends) { _depends_on_volatiles = depends; }

protected:
  bool _dirty = true;
  bool _depends_on_volatiles = false;
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
  virtual types::RuntimeTypeInfo const *type_info() = 0;
  // Read some bytes of memory (modulo the size of the address space) in the platform's preferred endianness
  virtual uint8_t read_mem_u8(uint32_t address) const = 0;
  virtual uint16_t read_mem_u16(uint32_t address) const = 0;
  virtual Value evaluate_variable(QStringView name) const = 0;
  virtual uint32_t cache_debug_variable_name(QStringView name) const = 0;
  virtual Value evaluate_debug_variable(uint32_t cache_index) const = 0;
};
struct ZeroEnvironment : public Environment {
  inline types::RuntimeTypeInfo const *type_info() override { return &_info; }
  inline uint8_t read_mem_u8(uint32_t address) const override { return 0; }
  inline uint16_t read_mem_u16(uint32_t address) const override { return 0; }
  inline Value evaluate_variable(QStringView name) const override { return VPrimitive::from_int(int16_t(0)); };
  inline uint32_t cache_debug_variable_name(QStringView name) const override { return 0; }
  inline Value evaluate_debug_variable(uint32_t cache_index) const override {
    return VPrimitive::from_int(int16_t(0));
  };

protected:
  types::RuntimeTypeInfo _info;
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

  // term.cache().version != _cache.version
  bool dirty();
  EvaluationCache cache() const;
  Value evaluate(CachePolicy mode, Environment &env);
  std::shared_ptr<Term> term();

private:
  EvaluationCache _cache = {};
  std::shared_ptr<Term> _term = nullptr;
};
} // namespace pepp::debug

