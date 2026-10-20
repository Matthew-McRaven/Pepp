#include "diagnostic_table.hpp"
#include <algorithm>
#include <string>

void pepp::tc::DiagnosticTable::add_message(support::LocationInterval i, std::string msg, Severity severity) {
  _raw.emplace(i, Diagnostic{std::move(msg), severity});
}

size_t pepp::tc::DiagnosticTable::count() const { return _raw.size(); }

size_t pepp::tc::DiagnosticTable::fatal_count() const {
  return std::ranges::count(_raw, Severity::Fatal, [](const auto &entry) { return entry.second.severity; });
}
