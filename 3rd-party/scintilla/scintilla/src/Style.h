#pragma once
// Scintilla source code edit control
/** @file Style.h
 ** Defines the font and colour style for a class of text.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <memory>
#include "Geometry.h"
#include "Platform.h"
#include "ScintillaTypes.h"
#include "scintilla_globals.h"

namespace Scintilla::Internal {

struct SCINTILLA_EXPORT FontSpecification {
  // fontName is allocated by a ViewStyle container object and may be null
  const char *fontName;
  int size;
  Scintilla::FontWeight weight = Scintilla::FontWeight::Normal;
  bool italic = false;
  Scintilla::CharacterSet characterSet = Scintilla::CharacterSet::Default;
  Scintilla::FontQuality extraFontFlag = Scintilla::FontQuality::QualityDefault;
  bool checkMonospaced = false;

  constexpr FontSpecification(const char *fontName_ = nullptr, int size_ = 10 * Scintilla::FontSizeMultiplier) noexcept
      : fontName(fontName_), size(size_) {}
  bool operator==(const FontSpecification &other) const noexcept;
  bool operator<(const FontSpecification &other) const noexcept;
};

struct SCINTILLA_EXPORT FontMeasurements {
  XYPOSITION ascent = 1;
  XYPOSITION descent = 1;
  XYPOSITION capitalHeight = 1; // Top of capital letter to baseline: ascent - internal leading
  XYPOSITION aveCharWidth = 1;
  XYPOSITION monospaceCharacterWidth = 1;
  XYPOSITION spaceWidth = 1;
  bool monospaceASCII = false;
  int sizeZoomed = 2;
};

class SCINTILLA_EXPORT Style : public FontSpecification, public FontMeasurements {
public:
  ColourRGBA fore;
  ColourRGBA back;
  bool eolFilled;
  bool underline;
  enum class CaseForce { mixed, upper, lower, camel };
  CaseForce caseForce;
  bool visible;
  bool changeable;
  bool hotspot;
  char invisibleRepresentation[5];

  std::shared_ptr<Font> font;

  Style(const char *fontName_ = nullptr) noexcept;
  void Copy(std::shared_ptr<Font> font_, const FontMeasurements &fm_) noexcept;
  bool IsProtected() const noexcept { return !(changeable && visible); }
};

} // namespace Scintilla::Internal
