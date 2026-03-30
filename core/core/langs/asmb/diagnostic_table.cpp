#include "diagnostic_table.hpp"
#include <string>

void pepp::tc::DiagnosticTable::add_message(support::LocationInterval i, std::string msg) { _raw.emplace(i, msg); }

size_t pepp::tc::DiagnosticTable::count() const { return _raw.size(); }
