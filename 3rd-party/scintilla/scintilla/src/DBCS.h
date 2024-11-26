#pragma once
// Scintilla source code edit control
/** @file DBCS.h
 ** Functions to handle DBCS double byte encodings like Shift-JIS.
 **/
// Copyright 2017 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "scintilla_globals.h"

namespace Scintilla::Internal {

constexpr bool IsDBCSCodePage(int codePage) noexcept {
  return codePage == 932 || codePage == 936 || codePage == 949 || codePage == 950 || codePage == 1361;
}

SCINTILLA_EXPORT bool DBCSIsLeadByte(int codePage, char ch) noexcept;
SCINTILLA_EXPORT bool IsDBCSValidSingleByte(int codePage, int ch) noexcept;
} // namespace Scintilla::Internal
