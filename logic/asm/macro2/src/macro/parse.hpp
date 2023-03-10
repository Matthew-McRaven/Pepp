#pragma once

#include <QtCore>
#include <tuple>

#include "macro_globals.hpp"

namespace macro {
// Analyze a macro's text body, and attempt to extract header information.
// Tuple returns 1) is the header well formed, 2) what is the macro's name,
// 3) how many arguments does the macro require?
MACRO_EXPORT std::tuple<bool, QString, quint8> analyze_macro_definition(QString macro_text);
}; // End namespace macro
