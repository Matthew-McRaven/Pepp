#include "scintillaasmeditbase.hpp"
#include "Geometry.h"
#include "LexillaAccess.h"
#include "SciLexer.h"
#include "ScintillaEditBase/PlatQt.h"

#include <QQmlEngine>
using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaAsmEditBase::ScintillaAsmEditBase(QQuickItem *parent) : ScintillaEditBase(parent) {
  // Handle adding breakpoints.
  connect(this, &ScintillaAsmEditBase::marginClicked, this, &ScintillaAsmEditBase::onMarginClicked);
  send(SCI_SETMARGINSENSITIVEN, 0, true);
  send(SCI_SETMARGINSENSITIVEN, 1, true);
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

Theme *ScintillaAsmEditBase::theme() const {
  QQmlEngine::setObjectOwnership(_theme, QQmlEngine::CppOwnership);
  return _theme;
}

void ScintillaAsmEditBase::setTheme(Theme *theme) {
  if (_theme == theme) return;
  _theme = theme;
  applyStyles();
  emit themeChanged();
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

sptr_t c2i(const QColor &color) { return ColourRGBAFromQColor(color).AsInteger(); }
void ScintillaAsmEditBase::applyStyles() {
  if (_theme == nullptr) return;
  send(SCI_STYLESETFORE, STYLE_DEFAULT, c2i(_theme->base()->foreground()));
  send(SCI_SETCARETFORE, c2i(_theme->base()->foreground()));
  send(SCI_STYLESETBACK, STYLE_DEFAULT, c2i(_theme->base()->background()));
  send(SCI_STYLECLEARALL, 0, 0);
  send(SCI_STYLESETFORE, commentStyle, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETBACK, commentStyle, c2i(_theme->comment()->background()));
  send(SCI_STYLESETFORE, errorStyle, c2i(_theme->error()->foreground()));
  send(SCI_STYLESETBACK, errorStyle, c2i(_theme->error()->background()));
  send(SCI_MARKERSETFORE, SC_MARK_CIRCLE, c2i(_theme->error()->background()));
  send(SCI_MARKERSETBACK, SC_MARK_CIRCLE, c2i(_theme->base()->background()));
}
