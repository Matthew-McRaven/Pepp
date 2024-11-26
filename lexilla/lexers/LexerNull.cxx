// Scintilla source code edit control
/** @file LexNull.cxx
 ** Lexer for no language. Used for plain text and unrecognized files.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <string_view>

#include "ILexer.h"
#include "SciLexer.h"
#include "Scintilla.h"

#include "Accessor.h"
#include "CharacterSet.h"
#include "LexAccessor.h"
#include "LexerModule.h"
#include "StyleContext.h"
#include "WordList.h"

using namespace Lexilla;

static void ColouriseNullDoc(Sci_PositionU startPos, Sci_Position length, int, WordList *[], Accessor &styler) {
  // Null language means all style bytes are 0 so just mark the end - no need to fill in.
  if (length > 0) {
    styler.StartAt(startPos + length - 1);
    styler.StartSegment(startPos + length - 1);
    styler.ColourTo(startPos + length - 1, 0);
  }
}

LexerModule lmNull(SCLEX_NULL, ColouriseNullDoc, "null");
