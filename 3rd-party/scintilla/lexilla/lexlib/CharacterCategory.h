#pragma once
// Scintilla source code edit control
/** @file CharacterCategory.h
 ** Returns the Unicode general category of a character.
 **/
// Copyright 2013 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include "lexilla_globals.h"

#include <vector>
namespace Lexilla {

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

LEXILLA_EXPORT CharacterCategory CategoriseCharacter(int character) noexcept;

// Common definitions of allowable characters in identifiers from UAX #31.
LEXILLA_EXPORT bool IsIdStart(int character) noexcept;
LEXILLA_EXPORT bool IsIdContinue(int character) noexcept;
LEXILLA_EXPORT bool IsXidStart(int character) noexcept;
LEXILLA_EXPORT bool IsXidContinue(int character) noexcept;

class LEXILLA_EXPORT CharacterCategoryMap {
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

} // namespace Lexilla
