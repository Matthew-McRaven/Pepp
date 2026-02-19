/*
 * Copyright (c) 2024-2026 J. Stanley Warford, Matthew McRaven
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "editbase.hpp"
#include <QQmlEngine>
#include "Geometry.h"
#include "LexillaAccess.h"
#include "SciLexer.h"
#include "ScintillaEditBase/PlatQt.h"
#include "core/arch/pep/isa/pep10.hpp"
#include "core/arch/pep/isa/pep9.hpp"
#include "core/arch/pep/uarch/pep.hpp"
#include "core/math/bitmanip/strings.hpp"
#include "fmt/ranges.h"
#include "settings/palette.hpp"
#include "settings/paletteitem.hpp"

using namespace Scintilla;
using namespace Scintilla::Internal;

EditBase::EditBase(QQuickItem *parent) : ScintillaEditBase(parent) {
  // Handle adding breakpoints.
  connect(this, &EditBase::marginClicked, this, &EditBase::onMarginClicked);
  send(SCI_SETMARGINSENSITIVEN, 0, true);
  send(SCI_SETMARGINSENSITIVEN, 1, true);
  send(SCI_SETMARGINSENSITIVEN, 2, true);
  // For code folding of comments and macros
  send(SCI_SETMARGINWIDTHN, 2, getCharWidth() * 2);
  send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
  send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
  send(SCI_MARKERDEFINE, conditionalBPStyle, SC_MARK_CIRCLEPLUS);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPEN, SC_MARK_MINUS);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDER, SC_MARK_PLUS);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERSUB, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERTAIL, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEREND, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDEROPENMID, SC_MARK_EMPTY);
  send(SCI_MARKERDEFINE, SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_EMPTY);
}

QString EditBase::lexerLanguage() const { return ""; }

void EditBase::onMarginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin) {
  // Margin 2 is used for folding
  if (margin == 2) {
    int line = send(SCI_LINEFROMPOSITION, position);
    int level = send(SCI_GETFOLDLEVEL, line);
    if (level & SC_FOLDLEVELHEADERFLAG) send(SCI_TOGGLEFOLD, line);
  } else { // Otherwise treat as BP modification.
    // Get line number from position
    int line = send(SCI_LINEFROMPOSITION, position, 0);
    int markers = send(SCI_MARKERGET, line);
    auto mask = BPStyleMask | conditionalBPStyleMask;
    emit modifyLine(line, markers & mask ? Action::RemoveBP : Action::AddBP);
  }
}

void EditBase::onLineAction(int line, Action action) {
  int markers = send(SCI_MARKERGET, line);
  auto exists = markers & (BPStyleMask | conditionalBPStyleMask);
  int start = send(SCI_POSITIONFROMLINE, line);
  int end = send(SCI_GETLINEENDPOSITION, line);
  if (action == Action::ToggleBP) {
    action = exists ? Action::RemoveBP : Action::AddBP;
  }
  switch (action) {
  case Action::AddBP:
    if (exists) return;
    send(SCI_MARKERADD, line, BPStyle);
    break;
  case Action::RemoveBP:
    send(SCI_MARKERDELETE, line, BPStyle);
    send(SCI_MARKERDELETE, line, conditionalBPStyle);
    break;
  case Action::ScrollTo: send(SCI_GOTOLINE, line); break;
  case Action::HighlightExclusive:
    send(SCI_GOTOLINE, line);
    send(SCI_INDICATORCLEARRANGE, 0, send(SCI_GETLENGTH));
    send(SCI_INDICATORFILLRANGE, start, end - start);
    break;
  case Action::MakeConditional:
    if (markers & conditionalBPStyleMask) return;
    send(SCI_MARKERADD, line, conditionalBPStyle);
    send(SCI_MARKERDELETE, line, BPStyle);
    break;
  case Action::MakeUnconditional:
    send(SCI_MARKERDELETE, line, conditionalBPStyle);
    send(SCI_MARKERADD, line, BPStyle);
    break;
  default: break;
  }
}

void EditBase::onClearAllBreakpoints() {
  send(SCI_MARKERDELETEALL, conditionalBPStyle);
  send(SCI_MARKERDELETEALL, BPStyle);
}

void EditBase::onRequestAllBreakpoints() {
  int totalLines = send(SCI_GETLINECOUNT);
  for (int line = 0; line < totalLines; ++line) {
    if (send(SCI_MARKERGET, line) & (conditionalBPStyleMask | BPStyleMask)) emit modifyLine(line, Action::AddBP);
  }
}

void EditBase::clearAllEOLAnnotations() { send(SCI_EOLANNOTATIONCLEARALL); }

void EditBase::setEOLAnnotationsVisible(int style) { send(SCI_EOLANNOTATIONSETVISIBLE, style); }

void EditBase::addEOLAnnotation(int line, const QString &annotation) {
  auto str = annotation.toStdString();
  send(SCI_EOLANNOTATIONSETTEXT, line, (sptr_t)str.c_str());
  send(SCI_EOLANNOTATIONSETSTYLE, line, (sptr_t)errorStyle);
}

void EditBase::clearAllInlineAnnotations() { send(SCI_ANNOTATIONCLEARALL); }

void EditBase::setInlineAnnotationsVisible(int style) { send(SCI_ANNOTATIONSETVISIBLE, style); }

void EditBase::addInlineAnnotation(int line, const QString &annotation) {
  auto str = annotation.toStdString();
  send(SCI_ANNOTATIONSETTEXT, line, (sptr_t)str.c_str());
  send(SCI_ANNOTATIONSETSTYLE, line, (sptr_t)STYLE_DEFAULT);
}

std::string pep9_mnemonics() {
  auto ret = fmt::format("{}", fmt::join(isa::Pep9::mnemonics(), " "));
  // lexer seems to lower-case before comparison, so our word list needs to be lower too.
  bits::to_lower_inplace(ret);
  return ret;
}
std::string pep9_directives() {
  std::string dirs;
  for (const auto &dir : isa::Pep9::legalDirectives()) dirs += "." + dir + " ";
  // lexer seems to lower-case before comparison, so our word list needs to be lower too.
  bits::to_lower_inplace(dirs);
  return dirs;
}

std::string pep10_mnemonics() {
  auto ret = fmt::format("{}", fmt::join(isa::Pep10::mnemonics(), " "));
  // lexer seems to lower-case before comparison, so our word list needs to be lower too.
  bits::to_lower_inplace(ret);
  return ret;
}
std::string pep10_directives() {
  std::string dirs;
  for (const auto &dir : isa::Pep10::legalDirectives()) dirs += "." + dir + " ";
  // lexer seems to lower-case before comparison, so our word list needs to be lower too.
  bits::to_lower_inplace(dirs);
  return dirs;
}

std::string pep9_1_byte_signals() {
  std::string ret;
  for (const auto &it : pepp::tc::arch::Pep9ByteBus::string_to_signal()) ret += it.first + " ";

  bits::to_lower_inplace(ret);
  return ret;
}

std::string pep9_2_byte_signals() {
  std::string ret;
  for (const auto &it : pepp::tc::arch::Pep9WordBus::string_to_signal()) ret += it.first + " ";

  bits::to_lower_inplace(ret);
  return ret;
}

std::string pep9_regs() {
  std::string ret;
  for (const auto &it : pepp::tc::arch::Pep9Registers::string_to_namedregister()) ret += it.first + " ";

  bits::to_lower_inplace(ret);
  return ret;
}

std::string pep9_csrs() {
  std::string ret;
  for (const auto &it : pepp::tc::arch::Pep9Registers::string_to_csr()) ret += it.first + " ";

  bits::to_lower_inplace(ret);
  return ret;
}

std::string pep9_1_byte_ucode() {
  static const auto str_signals = pep9_1_byte_signals();
  static const auto str_regs = pep9_regs();
  static const auto str_csr = pep9_csrs();
  return fmt::format("{} {} {} mem", str_regs, str_csr, str_signals);
}

std::string pep9_2_byte_ucode() {
  static const auto str_signals = pep9_2_byte_signals();
  static const auto str_regs = pep9_regs();
  static const auto str_csr = pep9_csrs();
  return fmt::format("{} {} {} mem", str_regs, str_csr, str_signals);
}

void EditBase::setLexerLanguage(const QString &language) {
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
  } else if (language == "Pep9Micro1") {
    lexer = Lexilla::MakeLexer("Pep9Micro");
    static const auto _signals = pep9_1_byte_ucode();
    lexer->WordListSet(0, _signals.c_str());
  } else if (language == "Pep9Micro2") {
    lexer = Lexilla::MakeLexer("Pep9Micro");
    static const auto _signals = pep9_2_byte_ucode();
    lexer->WordListSet(0, _signals.c_str());
  } else {
    // Unset previous lexer
    lexer = Lexilla::MakeLexer("null");
  }

  send(SCI_SETILEXER, /*unused*/ 0, (uintptr_t)lexer);
}

pepp::settings::Palette *EditBase::theme() const {
  QQmlEngine::setObjectOwnership(_theme, QQmlEngine::CppOwnership);
  return _theme;
}

void EditBase::setTheme(pepp::settings::Palette *theme) {
  if (_theme == theme) return;
  _theme = theme;
  applyStyles();
  emit themeChanged();
}

bool EditBase::lineNumbersVisible() const {
  auto currentWidth = send(SCI_GETMARGINWIDTHN, 0);
  return currentWidth > 0;
}

void EditBase::setLineNumbersVisible(bool visible) {
  if (lineNumbersVisible() == visible) return;
  auto width = getCharWidth() * 6;
  send(SCI_SETMARGINWIDTHN, 0, width);
  emit lineNumbersVisibleChanged();
}

int EditBase::caretBlink() const { return (int)send(SCI_GETCARETPERIOD); }

void EditBase::setCaretBlink(int blink) {
  if (caretBlink() == blink) return;
  else if (blink < 0) blink = 0; // Don't allow negative blink
  send(SCI_SETCARETPERIOD, blink);
  emit caretBlinkChanged();
}

void EditBase::applyStyles() {}

void EditBase::cut() { send(SCI_CUT); }

void EditBase::copy() { send(SCI_COPY); }

void EditBase::paste() { send(SCI_PASTE); }

void EditBase::undo() { send(SCI_UNDO); }

void EditBase::redo() { send(SCI_REDO); }

void EditBase::selectAll() { send(SCI_SELECTALL); }

sptr_t c2i(const QColor &color) { return ColourRGBAFromQColor(color).AsInteger(); }

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
