#include "scintillaasmeditbase.hpp"
#include <QQmlEngine>
#include "Geometry.h"
#include "LexillaAccess.h"
#include "SciLexer.h"
#include "ScintillaEditBase/PlatQt.h"
#include "isa/pep10.hpp"
#include "isa/pep9.hpp"
#include "settings/palette.hpp"
#include "settings/paletteitem.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

ScintillaAsmEditBase::ScintillaAsmEditBase(QQuickItem *parent) : ScintillaEditBase(parent) {
  // Handle adding breakpoints.
  connect(this, &ScintillaAsmEditBase::marginClicked, this, &ScintillaAsmEditBase::onMarginClicked);
  send(SCI_SETMARGINSENSITIVEN, 0, true);
  send(SCI_SETMARGINSENSITIVEN, 1, true);
  send(SCI_SETMARGINSENSITIVEN, 2, true);
  // For code folding of comments and macros
  send(SCI_SETMARGINWIDTHN, 2, getCharWidth() * 2);
  send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
  send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_PLUS);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY);
}

QString ScintillaAsmEditBase::lexerLanguage() const { return ""; }

void ScintillaAsmEditBase::onMarginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin) {
  // Margin 2 is used for folding
  if (margin == 2) {
    int line = send(SCI_LINEFROMPOSITION, position);
    int level = send(SCI_GETFOLDLEVEL, line);
    if (level & SC_FOLDLEVELHEADERFLAG) send(SCI_TOGGLEFOLD, line);
  } else { // Otherwise treat as BP modification.
    // Get line number from position
    int line = send(SCI_LINEFROMPOSITION, position, 0);
    int markers = send(SCI_MARKERGET, line);
    emit modifyLine(line, markers & (1 << SC_MARK_CIRCLE) ? Action::RemoveBP : Action::AddBP);
  }
}

void ScintillaAsmEditBase::onLineAction(int line, Action action) {
  int markers = send(SCI_MARKERGET, line);
  auto exists = markers & (1 << SC_MARK_CIRCLE);
  int start = send(SCI_POSITIONFROMLINE, line);
  int end = send(SCI_GETLINEENDPOSITION, line);
  switch (action) {
  // Toggle marker on the line
  case Action::ToggleBP: send(exists ? SCI_MARKERDELETE : SCI_MARKERADD, line, SC_MARK_CIRCLE); break;
  case Action::AddBP:
    if (exists) return;
    send(SCI_MARKERADD, line, SC_MARK_CIRCLE);
    break;
  case Action::RemoveBP: send(SCI_MARKERDELETE, line, SC_MARK_CIRCLE); break;
  case Action::ScrollTo: send(SCI_GOTOLINE, line); break;
  case Action::HighlightExclusive:
    send(SCI_GOTOLINE, line);
    send(SCI_INDICATORCLEARRANGE, 0, send(SCI_GETLENGTH));
    send(SCI_INDICATORFILLRANGE, start, end - start);
    break;
  default: break;
  }
}

void ScintillaAsmEditBase::onClearAllBreakpoints() { send(SCI_MARKERDELETEALL); }

void ScintillaAsmEditBase::onRequestAllBreakpoints() {
  int totalLines = send(SCI_GETLINECOUNT);

  for (int line = 0; line < totalLines; ++line)
    if (send(SCI_MARKERGET, line) & (1 << SC_MARK_CIRCLE)) modifyLine(line, Action::AddBP);
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

void ScintillaAsmEditBase::setEOLAnnotationsVisible(int style) { send(SCI_EOLANNOTATIONSETVISIBLE, style); }

void ScintillaAsmEditBase::addEOLAnnotation(int line, const QString &annotation) {
  auto str = annotation.toStdString();
  send(SCI_EOLANNOTATIONSETTEXT, line, (sptr_t)str.c_str());
  send(SCI_EOLANNOTATIONSETSTYLE, line, (sptr_t)errorStyle);
}

void ScintillaAsmEditBase::clearAllInlineAnnotations() { send(SCI_ANNOTATIONCLEARALL); }

void ScintillaAsmEditBase::setInlineAnnotationsVisible(int style) { send(SCI_ANNOTATIONSETVISIBLE, style); }

void ScintillaAsmEditBase::addInlineAnnotation(int line, const QString &annotation)
{
  auto str = annotation.toStdString();
  send(SCI_ANNOTATIONSETTEXT, line, (sptr_t)str.c_str());
  send(SCI_ANNOTATIONSETSTYLE, line, (sptr_t)STYLE_DEFAULT);
}

std::string pep9_mnemonics() {
  QStringList mnemonics_list;
  QMetaEnum mnemonic_enum = QMetaEnum::fromType<isa::Pep9::Mnemonic>();
  for (int it = 0; it < mnemonic_enum.keyCount(); it++) mnemonics_list << QString(mnemonic_enum.key(it)).toLower();
  return mnemonics_list.join(" ").toStdString();
}
std::string pep9_directives() {
  std::string dirs;
  for (const auto &dir : isa::Pep9::legalDirectives()) dirs += "." + dir.toLower().toStdString() + " ";
  return dirs;
}

std::string pep10_mnemonics() {
  QStringList mnemonics_list;
  QMetaEnum mnemonic_enum = QMetaEnum::fromType<isa::Pep10::Mnemonic>();
  for (int it = 0; it < mnemonic_enum.keyCount(); it++) mnemonics_list << QString(mnemonic_enum.key(it)).toLower();
  return mnemonics_list.join(" ").toStdString();
}
std::string pep10_directives() {
  std::string dirs;
  for (const auto &dir : isa::Pep10::legalDirectives()) dirs += "." + dir.toLower().toStdString() + " ";
  return dirs;
}

void ScintillaAsmEditBase::setLexerLanguage(const QString &language) {
  Scintilla::ILexer5 *lexer = nullptr;
  if (language == "Pep/9 ASM") {
    lexer = Lexilla::MakeLexer("Pep9ASM");
    static const auto mn_pep9 = pep9_mnemonics();
    static const auto dirs_pep9 = pep9_directives();
    lexer->WordListSet(0, mn_pep9.c_str());
    lexer->WordListSet(1, dirs_pep9.c_str());

  } else if (language == "Pep/10 ASM") {
    lexer = Lexilla::MakeLexer("Pep10ASM");
    static const auto mn_pep10 = pep10_mnemonics();
    static const auto dirs_pep10 = pep10_directives();
    lexer->WordListSet(0, mn_pep10.c_str());
    lexer->WordListSet(1, dirs_pep10.c_str());
  } else {
    // Unset previous lexer
    lexer = Lexilla::MakeLexer("null");
  }

  send(SCI_SETILEXER, /*unused*/ 0, (uintptr_t)lexer);
}

pepp::settings::Palette *ScintillaAsmEditBase::theme() const {
  QQmlEngine::setObjectOwnership(_theme, QQmlEngine::CppOwnership);
  return _theme;
}

void ScintillaAsmEditBase::setTheme(pepp::settings::Palette *theme) {
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
// Editor ignores alpha, so we need do do alpha compositing ourselves
// See: https://en.wikipedia.org/wiki/Alpha_compositing
sptr_t alphaBlend(const QColor &front, const QColor &back) {
  // Must do alpha math in [0,1]
  float fa = front.alphaF(), ba = back.alphaF(), fa1 = 1 - fa, bafa1 = ba * fa1;
  float frA = fa + bafa1;
  // Calculate colors using the over operator
  // Clip to [0,255] for ease of conversion
  auto rR = static_cast<quint8>((front.red() * fa + back.red() * bafa1) / frA);
  auto rG = static_cast<quint8>((front.green() * fa + back.green() * bafa1) / frA);
  auto rB = static_cast<quint8>((front.blue() * fa + back.blue() * bafa1) / frA);
  // Scale up alpha to [0,255] so it is in the same range as color components
  auto rA = static_cast<quint8>(frA * 255);
  // Premultiply colors by alpha so that the editor can drop alpha channel w/o loss of info.
  auto premult = QRgba64::fromArgb32(qRgba(rR, rG, rB, rA)).premultiplied();
  return c2i(QColor::fromRgba64(premult));
}
void ScintillaAsmEditBase::applyStyles() {
  // WARNING: If you anticipate a color having an alpha value, you will need to do the blending yourself!
  if (_theme == nullptr) return;
  auto baseBack = _theme->base()->background();
  send(SCI_STYLESETFORE, STYLE_DEFAULT, c2i(_theme->base()->foreground()));
  send(SCI_SETCARETFORE, c2i(_theme->base()->foreground()));
  send(SCI_STYLESETBACK, STYLE_DEFAULT, c2i(_theme->base()->background()));
  send(SCI_STYLECLEARALL, 0, 0);

  send(SCI_STYLESETFORE, symbolStyle, c2i(_theme->symbol()->foreground()));
  send(SCI_STYLESETITALIC, symbolStyle, _theme->symbol()->font().italic());
  send(SCI_STYLESETBOLD, symbolStyle, _theme->symbol()->font().bold());
  send(SCI_STYLESETBACK, symbolStyle, alphaBlend(_theme->symbol()->background(), baseBack));

  send(SCI_STYLESETFORE, mnemonicStyle, c2i(_theme->mnemonic()->foreground()));
  send(SCI_STYLESETITALIC, mnemonicStyle, _theme->mnemonic()->font().italic());
  send(SCI_STYLESETBOLD, mnemonicStyle, _theme->mnemonic()->font().bold());
  send(SCI_STYLESETBACK, mnemonicStyle, alphaBlend(_theme->mnemonic()->background(), baseBack));

  send(SCI_STYLESETFORE, directiveStyle, c2i(_theme->directive()->foreground()));
  send(SCI_STYLESETITALIC, directiveStyle, _theme->directive()->font().italic());
  send(SCI_STYLESETBOLD, directiveStyle, _theme->directive()->font().bold());
  send(SCI_STYLESETBACK, directiveStyle, alphaBlend(_theme->directive()->background(), baseBack));

  send(SCI_STYLESETFORE, macroStyle, c2i(_theme->macro()->foreground()));
  send(SCI_STYLESETITALIC, macroStyle, _theme->macro()->font().italic());
  send(SCI_STYLESETBOLD, macroStyle, _theme->macro()->font().bold());
  send(SCI_STYLESETBACK, macroStyle, alphaBlend(_theme->macro()->background(), baseBack));

  send(SCI_STYLESETFORE, charStyle, c2i(_theme->character()->foreground()));
  send(SCI_STYLESETITALIC, charStyle, _theme->character()->font().italic());
  send(SCI_STYLESETBOLD, charStyle, _theme->character()->font().bold());
  send(SCI_STYLESETBACK, charStyle, alphaBlend(_theme->character()->background(), baseBack));

  send(SCI_STYLESETFORE, stringStyle, c2i(_theme->string()->foreground()));
  send(SCI_STYLESETITALIC, stringStyle, _theme->string()->font().italic());
  send(SCI_STYLESETBOLD, stringStyle, _theme->string()->font().bold());
  send(SCI_STYLESETBACK, stringStyle, alphaBlend(_theme->string()->background(), baseBack));

  send(SCI_STYLESETFORE, SCE_PEPASM_MACRO_START, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, SCE_PEPASM_MACRO_START, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, SCE_PEPASM_MACRO_START, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, SCE_PEPASM_MACRO_START, alphaBlend(_theme->comment()->background(), baseBack));

  send(SCI_STYLESETFORE, SCE_PEPASM_MACRO_END, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, SCE_PEPASM_MACRO_END, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, SCE_PEPASM_MACRO_END, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, SCE_PEPASM_MACRO_END, alphaBlend(_theme->comment()->background(), baseBack));

  send(SCI_STYLESETFORE, commentStyle, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, commentStyle, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, commentStyle, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, commentStyle, alphaBlend(_theme->comment()->background(), baseBack));

  send(SCI_STYLESETFORE, SCE_PEPASM_COMMENT_LINE, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, SCE_PEPASM_COMMENT_LINE, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, SCE_PEPASM_COMMENT_LINE, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, SCE_PEPASM_COMMENT_LINE, alphaBlend(_theme->comment()->background(), baseBack));

  send(SCI_STYLESETFORE, errorStyle, c2i(_theme->error()->foreground()));
  send(SCI_STYLESETITALIC, errorStyle, _theme->error()->font().italic());
  send(SCI_STYLESETBOLD, errorStyle, _theme->error()->font().bold());
  send(SCI_STYLESETBACK, errorStyle, alphaBlend(_theme->error()->background(), baseBack));

  send(SCI_MARKERSETFORE, SC_MARK_CIRCLE, c2i(_theme->error()->background()));
  send(SCI_MARKERSETBACK, SC_MARK_CIRCLE, c2i(_theme->error()->background()));
  // Set the selection / highlighting for lines
  send(SCI_SETSELFORE, STYLE_DEFAULT, c2i(_theme->alternateBase()->foreground()));
  send(SCI_SETSELBACK, STYLE_DEFAULT, c2i(_theme->alternateBase()->background()));
  // Set the indicator style to a plain underline
  send(SCI_INDICSETSTYLE, 0, INDIC_ROUNDBOX);
  send(SCI_INDICSETFORE, 0, c2i(_theme->alternateBase()->foreground()));
  send(SCI_SETINDICATORVALUE, 0);
}

void ScintillaAsmEditBase::onCut() { send(SCI_CUT); }

void ScintillaAsmEditBase::onCopy() { send(SCI_COPY); }

void ScintillaAsmEditBase::onPaste() { send(SCI_PASTE); }

void ScintillaAsmEditBase::onUndo() { send(SCI_UNDO); }

void ScintillaAsmEditBase::onRedo() { send(SCI_REDO); }
