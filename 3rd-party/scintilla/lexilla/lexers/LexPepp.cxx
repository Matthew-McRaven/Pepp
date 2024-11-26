// Scintilla source code edit control
/** @file LexAsm.cxx
 ** Lexer for Pep/10, /9, and /8 assembler
 ** Written by Matthew McRVaen
 ** Derived from the ASM lexer.
 **/
// Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <QRegularExpression>
#include <assert.h>
#include <ctype.h>
#include <functional>
#include <map>
#include <set>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <string_view>
#include "CharacterSet.h"
#include "DefaultLexer.h"
#include "ILexer.h"
#include "LexAccessor.h"
#include "LexerModule.h"
#include "OptionSet.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "StyleContext.h"
#include "WordList.h"

using namespace Scintilla;
using namespace Lexilla;

static inline bool IsAWordChar(const int ch) { return isalnum(ch) || ch == '_'; }

static inline bool IsAWordStart(const int ch) { return isalpha(ch) || ch == '_'; }

static inline int LowerCase(int c) {
  if (c >= 'A' && c <= 'Z') return 'a' + c - 'A';
  return c;
}

// An individual named option for use in an OptionSet

// Options used for LexerPepAsm
struct OptionsPepAsm {
  bool fold;
  bool foldCompact;
  bool foldCommentMultiline;
  bool allowMacros;
  OptionsPepAsm() {
    fold = true;
    foldCompact = true;
    foldCommentMultiline = true;
    allowMacros = false;
  }
};

static const char *const PepAsmWordListDesc[] = {
    "Mnemonics",
    "Directives",
    0,
};

struct OptionSetPepAsm : public OptionSet<OptionsPepAsm> {
  OptionSetPepAsm() {
    DefineProperty("fold", &OptionsPepAsm::fold);

    DefineProperty("fold.asm.comment.multiline", &OptionsPepAsm::foldCommentMultiline,
                   "Set this property to 1 to enable folding multi-line comments.");


    DefineProperty("fold.compact", &OptionsPepAsm::foldCompact);

    DefineProperty("macros", &OptionsPepAsm::allowMacros);

    DefineWordListSets(PepAsmWordListDesc);
  }
};

namespace {}
class LexerPepAsm : public DefaultLexer {
  WordList mnemonics;
  WordList directives;
  OptionsPepAsm options;
  OptionSetPepAsm osAsm;

public:
  LexerPepAsm(const char *languageName_, int language_) : DefaultLexer(languageName_, language_) {}
  virtual ~LexerPepAsm() {}
  void SCI_METHOD Release() override { delete this; }
  int SCI_METHOD Version() const override { return lvRelease5; }
  const char *SCI_METHOD PropertyNames() override { return osAsm.PropertyNames(); }
  int SCI_METHOD PropertyType(const char *name) override { return osAsm.PropertyType(name); }
  const char *SCI_METHOD DescribeProperty(const char *name) override { return osAsm.DescribeProperty(name); }
  Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
  const char *SCI_METHOD PropertyGet(const char *key) override { return osAsm.PropertyGet(key); }
  const char *SCI_METHOD DescribeWordListSets() override { return osAsm.DescribeWordListSets(); }
  Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
  void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;
  void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

  void *SCI_METHOD PrivateCall(int, void *) override { return 0; }

  static ILexer5 *LexerFactoryPep10Asm() {
    auto ret = new LexerPepAsm("Pep/10 ASM", SCLEX_PEP10ASM);
    ret->PropertySet("macros", "1");
    return ret;
  }
  static ILexer5 *LexerFactoryPep9Asm() {
    auto ret = new LexerPepAsm("Pep/9 ASM", SCLEX_PEP9ASM);
    ret->PropertySet("macros", "0");
    return ret;
  }
};

Sci_Position SCI_METHOD LexerPepAsm::PropertySet(const char *key, const char *val) {
  if (osAsm.PropertySet(&options, key, val)) {
    return 0;
  }
  return -1;
}

Sci_Position SCI_METHOD LexerPepAsm::WordListSet(int n, const char *wl) {
  WordList *wordListN = 0;
  switch (n) {
  case 0: wordListN = &mnemonics; break;
  case 1: wordListN = &directives; break;
  }
  Sci_Position firstModification = -1;
  if (wordListN) {
    if (wordListN->Set(wl)) {
      firstModification = 0;
    }
  }
  return firstModification;
}

void SCI_METHOD LexerPepAsm::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
  LexAccessor styler(pAccess);

  const char commentCharacter = ';';
  static const auto macroStart = QRegularExpression(R"(^\s*;\s*@\w+)");
  static const auto macroEnd =
      QRegularExpression(R"(^\s*;\s*End\s*@\w+)", QRegularExpression::PatternOption::CaseInsensitiveOption);
  // Up to, but not including the current character, has only been white space.
  bool onlyWhiteSpace = true;
  StyleContext sc(startPos, length, initStyle, styler);

  char s[256];
  while (sc.More()) {
    if (sc.atLineStart) onlyWhiteSpace = true;
    // Actions + transition table.
    switch (sc.state) {
    case SCE_PEPASM_IDENTIFIER:
      if (sc.ch == ':') {
        sc.ChangeState(SCE_PEPASM_SYMBOL_DECL);
        sc.SetState(SCE_PEPASM_DEFAULT);
      } else if (!IsAWordChar(sc.ch)) {
        sc.GetCurrentLowered(s, sizeof(s));
        if (mnemonics.InList(s)) sc.ChangeState(SCE_PEPASM_MNEMONIC);
        sc.SetState(SCE_PEPASM_DEFAULT);
        continue;
      }
      break;
    case SCE_PEPASM_DIRECTIVE:
      if (!IsAWordChar(sc.ch)) {
        sc.GetCurrentLowered(s, sizeof(s));
        if (!directives.InList(s)) sc.ChangeState(SCE_PEPASM_IDENTIFIER);
        sc.SetState(SCE_PEPASM_DEFAULT);
        continue;
      }
      break;
    case SCE_PEPASM_MACRO:
      if (!IsAWordChar(sc.ch)) {
        sc.SetState(SCE_PEPASM_DEFAULT);
        continue;
      }
      break;
    case SCE_PEPASM_DEFAULT:
      if (options.allowMacros && sc.ch == '@') sc.SetState(SCE_PEPASM_MACRO);
      else if (sc.ch == ';' && !onlyWhiteSpace) sc.SetState(SCE_PEPASM_COMMENT);
      else if (sc.ch == ';' && onlyWhiteSpace) sc.SetState(SCE_PEPASM_COMMENT_LINE);
      else if (sc.ch == '\'') sc.SetState(SCE_PEPASM_CHARACTER);
      else if (sc.ch == '"') sc.SetState(SCE_PEPASM_STRING);
      else if (sc.ch == '.') sc.SetState(SCE_PEPASM_DIRECTIVE);
      else if (IsAWordStart(sc.ch)) sc.SetState(SCE_PEPASM_IDENTIFIER);
      else if (sc.atLineEnd) sc.SetState(SCE_PEPASM_DEFAULT);
      break;
    case SCE_PEPASM_CHARACTER:
      if (sc.ch == '\'') sc.ForwardSetState(SCE_PEPASM_DEFAULT);
      else if (sc.atLineEnd) sc.SetState(SCE_PEPASM_DEFAULT);
      break;
    case SCE_PEPASM_STRING:
      if (sc.ch == '"' && sc.chPrev != '\\') sc.ForwardSetState(SCE_PEPASM_DEFAULT);
      else if (sc.atLineEnd) sc.SetState(SCE_PEPASM_DEFAULT);
    case SCE_PEPASM_COMMENT_LINE: [[fallthrough]];
    case SCE_PEPASM_COMMENT:
      if (sc.atLineEnd) {
        sc.GetCurrentLowered(s, sizeof(s));
        if (macroStart.match(s).hasMatch()) sc.ChangeState(SCE_PEPASM_MACRO_START);
        else if (macroEnd.match(s).hasMatch()) sc.ChangeState(SCE_PEPASM_MACRO_END);
        sc.ForwardSetState(SCE_PEPASM_DEFAULT);
        continue;
      }
      break;
    default: break;
    }
    // Evaluate after loop so that "    ;" evaluates
    onlyWhiteSpace &= IsASpace(sc.ch);
    sc.Forward();
  }
  sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "else".

void SCI_METHOD LexerPepAsm::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

  if (!options.fold) return;

  LexAccessor styler(pAccess);

  Sci_PositionU endPos = startPos + length;
  int visibleChars = 0;
  Sci_Position lineCurrent = styler.GetLine(startPos);
  int levelCurrent = SC_FOLDLEVELBASE;
  if (lineCurrent > 0) levelCurrent = styler.LevelAt(lineCurrent - 1) >> 16;
  int levelNext = levelCurrent;
  char chNext = styler[startPos];
  int styleNext = styler.StyleAt(startPos);
  int style = initStyle;
  char word[100];
  int wordlen = 0;
  const bool userDefinedFoldMarkers = false; // !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
  for (Sci_PositionU i = startPos; i < endPos; i++) {
    char ch = chNext;
    chNext = styler.SafeGetCharAt(i + 1);
    int stylePrev = style;
    style = styleNext;
    styleNext = styler.StyleAt(i + 1);
    bool atEOL = (ch == '\r' && chNext != '\n') || (ch == '\n');
    bool atSOL = (styler.SafeGetCharAt(i - 1) == '\n');
    if (style == SCE_PEPASM_MACRO_START && stylePrev != SCE_PEPASM_MACRO_START) levelNext++;
    else if (style == SCE_PEPASM_MACRO_END && styleNext != SCE_PEPASM_MACRO_END) levelNext--;
    else if (options.foldCommentMultiline && style == SCE_PEPASM_COMMENT_LINE) {
      if (stylePrev != SCE_PEPASM_COMMENT_LINE) levelNext++;
      else if (atEOL && styleNext != SCE_PEPASM_COMMENT_LINE) levelNext--;
    }

    if (!IsASpace(ch)) visibleChars++;
    if (atEOL || (i == endPos - 1)) {
      int levelUse = levelCurrent;
      if (levelUse < levelNext) levelUse |= SC_FOLDLEVELHEADERFLAG;
      if (visibleChars == 0 && options.foldCompact) levelUse |= SC_FOLDLEVELWHITEFLAG;
      styler.SetLevel(lineCurrent, levelUse | (levelNext << 16));
      lineCurrent++;
      levelCurrent = levelNext;
      visibleChars = 0;
    }
  }
}

LexerModule lmPep10(SCLEX_PEP10ASM, LexerPepAsm::LexerFactoryPep10Asm, "Pep10ASM", PepAsmWordListDesc);
LexerModule lmPep9(SCLEX_PEP9ASM, LexerPepAsm::LexerFactoryPep9Asm, "Pep9ASM", PepAsmWordListDesc);
