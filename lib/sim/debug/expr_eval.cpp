#include "expr_eval.hpp"
#include "expr_ast.hpp"

pepp::debug::CachedEvaluator::CachedEvaluator(std::shared_ptr<Term> term)
    : _cache(term->cached()), _term(std::move(term)) {}

bool pepp::debug::CachedEvaluator::dirty() {
  if (!_term) return false;
  return _cache.version != _term->cached().version;
}

std::shared_ptr<pepp::debug::Term> pepp::debug::CachedEvaluator::term() { return _term; }

pepp::debug::EvaluationCache pepp::debug::CachedEvaluator::cache() const { return _cache; }

pepp::debug::Value pepp::debug::CachedEvaluator::evaluate(CachePolicy mode, Environment &env) {
  if (!_term) return VNever{};

  auto other = _term->cached();
  auto hasCachedValue = _cache.value.has_value();
  bool isDirty = _cache.version == other.version && !_term->dirty();
  switch (mode) {
  case CachePolicy::UseDirtyAlways:
    if (hasCachedValue) return *_cache.value;
    break;
  case CachePolicy::UseAlways:
    if (hasCachedValue && !isDirty) return *_cache.value;
    break;
  case CachePolicy::UseNonVolatiles:
    if (hasCachedValue && !isDirty && !_cache.depends_on_volatiles) return *_cache.value;
    break;
  case CachePolicy::UseNever: break;
  }
  (void)_term->evaluate(mode, env); // Cause cached value to be updated
  _cache = _term->cached();
  return *_cache.value;
}
