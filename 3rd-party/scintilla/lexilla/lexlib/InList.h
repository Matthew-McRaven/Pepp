#pragma once
// Scintilla source code edit control
/** @file InList.h
 ** Check if a string is in a list.
 **/
// Copyright 2024 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <string_view>
#include "lexilla_globals.h"

namespace Lexilla {

LEXILLA_EXPORT bool InList(std::string_view value, std::initializer_list<std::string_view> list) noexcept;
LEXILLA_EXPORT bool InListCaseInsensitive(std::string_view value,
                                          std::initializer_list<std::string_view> list) noexcept;

} // namespace Lexilla
