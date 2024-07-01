#include "scintillaasmeditbase.hpp"
#include "Geometry.h"
#include "LexillaAccess.h"
#include "SciLexer.h"
#include "ScintillaEditBase/PlatQt.h"
using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaAsmEditBase::ScintillaAsmEditBase(QQuickItem *parent) : ScintillaEditBase(parent) {
  // Handle adding breakpoints.
  connect(this, &ScintillaAsmEditBase::marginClicked, this, &ScintillaAsmEditBase::onMarginClicked);
  send(SCI_SETMARGINSENSITIVEN, 0, true);
  send(SCI_SETMARGINSENSITIVEN, 1, true);

  _text = send(SCI_STYLEGETFORE, STYLE_DEFAULT, 0);
  _bg = send(SCI_STYLEGETBACK, STYLE_DEFAULT, 0);
  _errFg = send(SCI_STYLEGETFORE, errorStyle, 0);
  _errBg = send(SCI_STYLEGETBACK, errorStyle, 0);
  _commentFg = send(SCI_STYLEGETFORE, commentStyle, 0);
  _commentBg = send(SCI_STYLEGETBACK, commentStyle, 0);
}

QString ScintillaAsmEditBase::lexerLanguage() const { return ""; }

void ScintillaAsmEditBase::onMarginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin) {
  // Get line number from position
  int line = send(SCI_LINEFROMPOSITION, position, 0);

  // Toggle marker on the line
  int markers = send(SCI_MARKERGET, line);
  auto msg = markers & (1 << SC_MARK_CIRCLE) ? SCI_MARKERDELETE : SCI_MARKERADD;
  send(msg, line, SC_MARK_CIRCLE);
}

/* // I actually think this is a bad idea, but keeping code for reference.
void ScintillaEditBase::removeMarkersOnModified(Scintilla::ModificationFlags type, Scintilla::Position position,
                                                Scintilla::Position length, Scintilla::Position linesAdded,
                                                const QByteArray &text, Scintilla::Position line,
                                                Scintilla::FoldLevel foldNow, Scintilla::FoldLevel foldPrev) {
  // Lines added is unsigned, but can contain negative values if lines were deleted.
  // Cast to avoid signed'ness issues / trivially being true.
  if (!FlagSet(type, ModificationFlags::DeleteText) || std::make_signed<Scintilla::Position>::type(linesAdded) >= 0)
    return;
  int startLine = send(SCI_LINEFROMPOSITION, position);
  // markers are merged when lines are deleted, so we may remove markers we wished to keep.
  for (int it = startLine + linesAdded; it <= startLine; ++it) send(SCI_MARKERDELETE, it, -1);
}*/

void ScintillaAsmEditBase::clearAllEOLAnnotations() { send(SCI_EOLANNOTATIONCLEARALL); }

void ScintillaAsmEditBase::setEOLAnnotationsVisibile(int style) { send(SCI_EOLANNOTATIONSETVISIBLE, style); }

void ScintillaAsmEditBase::addEOLAnnotation(int line, const QString &annotation) {
  auto str = annotation.toStdString();
  send(SCI_EOLANNOTATIONSETTEXT, line, (sptr_t)str.c_str());
  send(SCI_EOLANNOTATIONSETSTYLE, line, (sptr_t)errorStyle);
}

void ScintillaAsmEditBase::setLexerLanguage(const QString &language) {
  auto lexer = Lexilla::MakeLexer("Pep10ASM");
  send(SCI_SETILEXER, /*unused*/ 0, (uintptr_t)lexer);
}

QColor ScintillaAsmEditBase::textColor() const {
  auto ca = ColourRGBA::FromIpRGB(send(SCI_STYLEGETFORE, STYLE_DEFAULT));
  return QColorFromColourRGBA(ca);
}

void ScintillaAsmEditBase::setTextColor(const QColor &color) {
  if (textColor() == color) return;
  _text = ColourRGBAFromQColor(color).AsInteger();
  applyStyles();
  emit colorChanged();
}

QColor ScintillaAsmEditBase::backgroundColor() const {
  auto ca = ColourRGBA::FromIpRGB(send(SCI_STYLEGETBACK, STYLE_DEFAULT));
  return QColorFromColourRGBA(ca);
}

void ScintillaAsmEditBase::setBackgroundColor(const QColor &color) {
  if (backgroundColor() == color) return;
  _bg = ColourRGBAFromQColor(color).AsInteger();
  applyStyles();
  emit colorChanged();
}

QColor ScintillaAsmEditBase::errorForegroundColor() const {
  auto ca = ColourRGBA::FromIpRGB(send(SCI_STYLEGETFORE, errorStyle));
  return QColorFromColourRGBA(ca);
}

void ScintillaAsmEditBase::setErrorForegroundColor(const QColor &color) {
  if (errorForegroundColor() == color) return;
  _errFg = ColourRGBAFromQColor(color).AsInteger();
  applyStyles();
  emit colorChanged();
}

QColor ScintillaAsmEditBase::errorBackgroundColor() const {
  auto ca = ColourRGBA::FromIpRGB(send(SCI_STYLEGETBACK, errorStyle));
  return QColorFromColourRGBA(ca);
}

void ScintillaAsmEditBase::setErrorBackgroundColor(const QColor &color) {
  if (errorBackgroundColor() == color) return;
  _errBg = ColourRGBAFromQColor(color).AsInteger();
  applyStyles();
  emit colorChanged();
}

QColor ScintillaAsmEditBase::commentForegroundColor() const {
  auto ca = ColourRGBA::FromIpRGB(send(SCI_STYLEGETFORE, commentStyle));
  return QColorFromColourRGBA(ca);
}

void ScintillaAsmEditBase::setCommentForegroundColor(const QColor &color) {
  if (commentForegroundColor() == color) return;
  _commentFg = ColourRGBAFromQColor(color).AsInteger();
  applyStyles();
  emit colorChanged();
}

QColor ScintillaAsmEditBase::commentBackgroundColor() const {
  auto ca = ColourRGBA::FromIpRGB(send(SCI_STYLEGETBACK, commentStyle));
  return QColorFromColourRGBA(ca);
}

void ScintillaAsmEditBase::setCommentBackgroundColor(const QColor &color) {
  if (commentBackgroundColor() == color) return;
  _commentBg = ColourRGBAFromQColor(color).AsInteger();
  applyStyles();
  emit colorChanged();
}

bool ScintillaAsmEditBase::lineNumbersVisible() const {
  auto currentWidth = send(SCI_GETMARGINWIDTHN, 0);
  return currentWidth > 0;
}

void ScintillaAsmEditBase::setLineNumbersVisible(bool visible) {

  if (lineNumbersVisible() == visible) return;
  auto width = getCharWidth() * 6;
  send(SCI_SETMARGINWIDTHN, 0, width);
  emit lineNumbersVisibleChanged();
}

void ScintillaAsmEditBase::applyStyles() {
  send(SCI_STYLESETFORE, STYLE_DEFAULT, _text);
  send(SCI_SETCARETFORE, _text);
  send(SCI_STYLESETBACK, STYLE_DEFAULT, _bg);
  send(SCI_STYLECLEARALL, 0, 0);
  // For comments
  send(SCI_STYLESETFORE, commentStyle, _commentFg);
  send(SCI_STYLESETBACK, commentStyle, _commentBg);
  // For EOL error annotations
  send(SCI_STYLESETFORE, errorStyle, _errFg);
  send(SCI_STYLESETBACK, errorStyle, _errBg);
  // For breakpoints
  send(SCI_MARKERSETFORE, SC_MARK_CIRCLE, _bg);
  send(SCI_MARKERSETBACK, SC_MARK_CIRCLE, _errBg);
}
