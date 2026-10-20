#pragma once

#include <map>
#include <string>
#include "core/compile/source/location.hpp"
namespace pepp::tc {

// Any part of compilation / assembly / linking that can fail should take a DiagnosticTable& as their first argument.
// e.g., parsing can fail because the user entered an invalid program, while assign_addresses cannot because the
// previous passes caught all possible errors. We chose to ignore system errors (e.g., ran out of memory) to reduce the
// number of failable functions.
// Must be ordered so than can be compared by </<=
enum class Severity {
  Warning, // The output will simulate correctly, but debugging features may be inaccurate.
  Error,   // The output is wrong, but its layout is intact, so later stages may still run.
  Fatal,   // Later stages cannot run meaningfully.
};
struct Diagnostic {
  std::string message;
  Severity severity;
};

class DiagnosticTable {
public:
  void add_message(pepp::tc::support::LocationInterval, std::string, Severity);
  auto begin() { return _raw.cbegin(); }
  auto end() { return _raw.cend(); }
  auto cbegin() const { return _raw.cbegin(); }
  auto cend() const { return _raw.cend(); }
  auto overlapping_interval(support::LocationInterval i) const {
    auto lb = i.lower().valid() ? _raw.lower_bound(i.lower()) : _raw.cbegin();
    auto ub = i.upper().valid() ? _raw.upper_bound(i.upper()) : _raw.cend();
    return std::pair{lb, ub};
  }
  size_t count() const;
  // Number of Diagnostics at or above a given severity.
  size_t count(Severity minimum) const;

private:
  std::multimap<pepp::tc::support::LocationInterval, Diagnostic> _raw;
};
} // namespace pepp::tc
