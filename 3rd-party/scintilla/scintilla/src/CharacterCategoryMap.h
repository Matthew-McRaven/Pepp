#pragma once
// Scintilla source code edit control
/** @file CharacterCategoryMap.h
 ** Returns the Unicode general category of a character.
 ** Similar code to Lexilla's lexilla/lexlib/CharacterCategory.h but renamed
 ** to avoid problems with builds that statically include both Scintilla and Lexilla.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <vector>
#include "scintilla_globals.h"

namespace Scintilla::Internal {

// clang-format off
enum CharacterCategory {
	ccLu, ccLl, ccLt, ccLm, ccLo,
	ccMn, ccMc, ccMe,
	ccNd, ccNl, ccNo,
	ccPc, ccPd, ccPs, ccPe, ccPi, ccPf, ccPo,
	ccSm, ccSc, ccSk, ccSo,
	ccZs, ccZl, ccZp,
	ccCc, ccCf, ccCs, ccCo, ccCn
};
// clang-format on

SCINTILLA_EXPORT CharacterCategory CategoriseCharacter(int character) noexcept;

// Common definitions of allowable characters in identifiers from UAX #31.
SCINTILLA_EXPORT bool IsIdStart(int character) noexcept;
SCINTILLA_EXPORT bool IsIdContinue(int character) noexcept;
SCINTILLA_EXPORT bool IsXidStart(int character) noexcept;
SCINTILLA_EXPORT bool IsXidContinue(int character) noexcept;

class SCINTILLA_EXPORT CharacterCategoryMap {
private:
  std::vector<unsigned char> dense;

public:
  CharacterCategoryMap();
  CharacterCategory CategoryFor(int character) const noexcept {
    if (static_cast<size_t>(character) < dense.size()) {
      return static_cast<CharacterCategory>(dense[character]);
    } else {
      // binary search through ranges
      return CategoriseCharacter(character);
    }
  }
  int Size() const noexcept;
  void Optimize(int countCharacters);
};
} // namespace Scintilla::Internal
