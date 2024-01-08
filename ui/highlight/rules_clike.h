#pragma once
#include "rules.hpp"

namespace highlight {
// Subsumes __old/chighlighter.cpp; See: babb87b6df834d75cfc133a36a4c19e931064079
QList<Rule> rules_c();
QList<Rule> rules_cpp();
}
