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

// Options used for LexerPepMicro
struct OptionsPepMicro {
  bool allowSymbols = false;
  OptionsPepMicro() {}
};

static const char *const PepMicroWordListDesc[] = {
    "signals",
    0,
};

struct OptionsSetPepMicro : public OptionSet<OptionsPepMicro> {
  OptionsSetPepMicro() {
    DefineProperty("symbols", &OptionsPepMicro::allowSymbols);
    DefineWordListSets(PepMicroWordListDesc);
  }
};

class LexerPepMicro : public DefaultLexer {
  WordList _signals;
  OptionsPepMicro options;
  OptionsSetPepMicro osMicro;

public:
  LexerPepMicro(const char *languageName_, int language_) : DefaultLexer(languageName_, language_) {}
  virtual ~LexerPepMicro() {}
  void SCI_METHOD Release() override { delete this; }
  int SCI_METHOD Version() const override { return lvRelease5; }
  const char *SCI_METHOD PropertyNames() override { return osMicro.PropertyNames(); }
  int SCI_METHOD PropertyType(const char *name) override { return osMicro.PropertyType(name); }
  const char *SCI_METHOD DescribeProperty(const char *name) override { return osMicro.DescribeProperty(name); }
  Sci_Position SCI_METHOD PropertySet(const char *key, const char *val) override;
  const char *SCI_METHOD PropertyGet(const char *key) override { return osMicro.PropertyGet(key); }
  const char *SCI_METHOD DescribeWordListSets() override { return osMicro.DescribeWordListSets(); }
  Sci_Position SCI_METHOD WordListSet(int n, const char *wl) override;
  void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) override;

  void *SCI_METHOD PrivateCall(int, void *) override { return 0; }

  static ILexer5 *LexerFactoryPep10Micro() {
    auto ret = new LexerPepMicro("Pep/10 Micro", SCLEX_PEP10MICRO);
    ret->PropertySet("symbols", "0");
    return ret;
  }
  static ILexer5 *LexerFactoryPep9Micro() {
    auto ret = new LexerPepMicro("Pep/9 Micro", SCLEX_PEP9MICRO);
    ret->PropertySet("symbols", "0");
    return ret;
  }
};

Sci_Position SCI_METHOD LexerPepMicro::PropertySet(const char *key, const char *val) {
  if (osMicro.PropertySet(&options, key, val)) return 0;
  return -1;
}

Sci_Position SCI_METHOD LexerPepMicro::WordListSet(int n, const char *wl) {
  WordList *wordListN = 0;
  switch (n) {
  case 0: wordListN = &_signals; break;
  }
  Sci_Position firstModification = -1;
  if (wordListN) {
    if (wordListN->Set(wl)) {
      firstModification = 0;
    }
  }
  return firstModification;
}

void SCI_METHOD LexerPepMicro::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
  LexAccessor styler(pAccess);

  static const char *unitpre = "unitpre:";
  static const char *unitpost = "unitpost:";
  // Optional `symbol:`, followed by a required mnemonic.

  // Up to, but not including the current character, has only been white space.
  bool onlyWhiteSpace = true;
  StyleContext sc(startPos, length, initStyle, styler);

  char s[256];
  while (sc.More()) {
    if (sc.atLineStart) {
      onlyWhiteSpace = true;
      // Prevent comments from bleeding one line to the next.
      sc.ChangeState(SCE_PEPMICRO_DEFAULT);
    }

    // Actions + transition table.
    switch (sc.state) {
    case SCE_PEPMICRO_DEFAULT: {
      if (sc.ch == '/') {
        if (sc.chNext == '/') {
          if (onlyWhiteSpace) sc.SetState(SCE_PEPMICRO_COMMENT_LINE);
          else sc.SetState(SCE_PEPMICRO_COMMENT);
        }
      } else if (IsAWordStart(sc.ch)) sc.SetState(SCE_PEPMICRO_IDENTIFIER);
      else if (IsADigit(sc.ch)) sc.SetState(SCE_PEPMICRO_INTEGER);
      else if (sc.atLineEnd) sc.SetState(SCE_PEPMICRO_DEFAULT);
    } break;
    case SCE_PEPMICRO_IDENTIFIER:
      if (sc.ch == ':') {
        sc.GetCurrentLowered(s, sizeof(s));
        // check if unit pre or post
        if (strcmp(s, unitpre) == 0) {
          sc.ChangeState(SCE_PEPMICRO_PREPOST);
          sc.SetState(SCE_PEPMICRO_DEFAULT);
        } else if (strcmp(s, unitpost) == 0) {
          sc.ChangeState(SCE_PEPMICRO_PREPOST);
          sc.SetState(SCE_PEPMICRO_DEFAULT);
        } else if (options.allowSymbols) {
          sc.ChangeState(SCE_PEPMICRO_SYMBOL_DECL);
          sc.SetState(SCE_PEPMICRO_DEFAULT);
        } else {
          sc.ChangeState(SCE_PEPMICRO_DEFAULT);
          sc.SetState(SCE_PEPMICRO_DEFAULT);
        }
      } else if (!IsAWordChar(sc.ch)) {
        sc.GetCurrentLowered(s, sizeof(s));
        if (!_signals.InList(s)) sc.ChangeState(SCE_PEPMICRO_DEFAULT);
        sc.ForwardSetState(SCE_PEPMICRO_DEFAULT);
        continue;
      }
      break;
    case SCE_PEPMICRO_INTEGER:
      if (!IsAHeXDigit(sc.ch) && sc.ch != 'x' && sc.ch != 'X') sc.SetState(SCE_PEPMICRO_DEFAULT);
      break;
    case SCE_PEPMICRO_COMMENT: [[fallthrough]];
    case SCE_PEPMICRO_COMMENT_LINE:
      if (sc.atLineEnd || sc.ch == 0) {
        sc.ForwardSetState(SCE_PEPMICRO_DEFAULT);
        continue;
      }
      break;
    default: break;
    }
    // Evaluate after loop so that "    ;" evaluates to false.
    onlyWhiteSpace &= IsASpace(sc.ch);
    sc.Forward();
  }
  sc.Complete();
}

LexerModule lmPep10micro(SCLEX_PEP10MICRO, LexerPepMicro::LexerFactoryPep10Micro, "Pep10Micro", PepMicroWordListDesc);
LexerModule lmPep9micro(SCLEX_PEP9MICRO, LexerPepMicro::LexerFactoryPep9Micro, "Pep9Micro", PepMicroWordListDesc);
