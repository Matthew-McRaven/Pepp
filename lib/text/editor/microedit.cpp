#include "microedit.hpp"

MicroEdit::MicroEdit(QQuickItem *parent) : EditBase(parent) {
  // Margin 0 already used for line numbers, so use margin 1 for breakpoints, and margin 2 for cycle numbers
  send(SCI_SETMARGINSENSITIVEN, 1, true);
  send(SCI_SETMARGINSENSITIVEN, 2, true);
  // For code folding of comments and macros
  send(SCI_SETMARGINWIDTHN, 2, getCharWidth() * 2);
  send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_RTEXT);
  send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
}

pepp::LineNumbers *MicroEdit::lineNumbers() const { return _lineNumber; }

void MicroEdit::setLineNumbers(pepp::LineNumbers *lineNumber) {
  if (_lineNumber == lineNumber) return;
  _lineNumber = lineNumber;
  const int lineCount = static_cast<int>(send(SCI_GETLINECOUNT, 0, 0));
  for (int line = 0; line < lineCount; ++line) {
    std::optional<int> cycle_num = std::nullopt;
    if (_lineNumber) cycle_num = _lineNumber->l2a.address(line);
    std::string cycle_num_str = cycle_num ? std::to_string(*cycle_num + 1) : "";
    send(SCI_MARGINSETTEXT, line, reinterpret_cast<sptr_t>(cycle_num_str.c_str()));
    send(SCI_MARGINSETSTYLE, line, cycleNumStyle);
  }
  emit lineNumberChanged();
}

void MicroEdit::applyStyles() {
  // WARNING: If you anticipate a color having an alpha value, you will need to do the blending yourself!
  if (_theme == nullptr) return;
  auto baseBack = _theme->base()->background();
  send(SCI_STYLESETFORE, STYLE_DEFAULT, c2i(_theme->base()->foreground()));
  send(SCI_SETCARETFORE, c2i(_theme->base()->foreground()));
  setStylesFont(_theme->baseMono()->font(), STYLE_DEFAULT);
  send(SCI_STYLESETBACK, STYLE_DEFAULT, c2i(_theme->base()->background()));
  send(SCI_STYLECLEARALL, 0, 0);

  send(SCI_STYLESETFORE, integerStyle, c2i(_theme->symbol()->foreground()));
  send(SCI_STYLESETBACK, integerStyle, alphaBlend(_theme->symbol()->background(), baseBack));

  send(SCI_STYLESETFORE, signalStyle, c2i(_theme->mnemonic()->foreground()));
  send(SCI_STYLESETITALIC, signalStyle, _theme->mnemonic()->font().italic());
  send(SCI_STYLESETBOLD, signalStyle, _theme->mnemonic()->font().bold());
  send(SCI_STYLESETBACK, signalStyle, alphaBlend(_theme->mnemonic()->background(), baseBack));

  send(SCI_STYLESETFORE, prepostStyle, c2i(_theme->mnemonic()->foreground()));
  send(SCI_STYLESETITALIC, prepostStyle, _theme->mnemonic()->font().italic());
  send(SCI_STYLESETBOLD, prepostStyle, _theme->mnemonic()->font().bold());
  send(SCI_STYLESETBACK, prepostStyle, alphaBlend(_theme->mnemonic()->background(), baseBack));

  send(SCI_STYLESETFORE, symbolStyle, c2i(_theme->character()->foreground()));
  send(SCI_STYLESETITALIC, symbolStyle, _theme->character()->font().italic());
  send(SCI_STYLESETBOLD, symbolStyle, _theme->character()->font().bold());
  send(SCI_STYLESETBACK, symbolStyle, alphaBlend(_theme->character()->background(), baseBack));

  send(SCI_STYLESETFORE, commentStyle, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, commentStyle, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, commentStyle, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, commentStyle, alphaBlend(_theme->comment()->background(), baseBack));

  send(SCI_STYLESETFORE, SCE_PEPMICRO_COMMENT_LINE, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, SCE_PEPMICRO_COMMENT_LINE, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, SCE_PEPMICRO_COMMENT_LINE, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, SCE_PEPMICRO_COMMENT_LINE, alphaBlend(_theme->comment()->background(), baseBack));

  send(SCI_STYLESETFORE, errorStyle, c2i(_theme->error()->foreground()));
  send(SCI_STYLESETITALIC, errorStyle, _theme->error()->font().italic());
  send(SCI_STYLESETBOLD, errorStyle, _theme->error()->font().bold());
  send(SCI_STYLESETBACK, errorStyle, alphaBlend(_theme->error()->background(), baseBack));


  send(SCI_MARKERSETFORE, BPStyle, c2i(_theme->error()->foreground()));
  send(SCI_MARKERSETBACK, BPStyle, c2i(_theme->error()->background()));
  send(SCI_MARKERSETFORE, conditionalBPStyle, c2i(_theme->error()->background()));
  send(SCI_MARKERSETBACK, conditionalBPStyle, c2i(_theme->error()->foreground()));

  send(SCI_STYLESETFORE, cycleNumStyle, c2i(_theme->base()->foreground()));
  send(SCI_STYLESETBACK, cycleNumStyle, c2i(_theme->midlight()->background()));

  // Ensure fold margin is tied to the theme. Must set normal+hi else checkerboard ensues.
  send(SCI_SETFOLDMARGINCOLOUR, true, c2i(_theme->midlight()->background()));
  send(SCI_SETFOLDMARGINHICOLOUR, true, c2i(_theme->midlight()->background()));
  send(SCI_SETMARGINBACKN, 2, c2i(_theme->midlight()->background()));
  // Set the selection / highlighting for lines
  send(SCI_SETSELFORE, STYLE_DEFAULT, c2i(_theme->highlight()->foreground()));
  send(SCI_SETSELBACK, STYLE_DEFAULT, c2i(_theme->highlight()->background()));

  // Set the indicator style to a plain underline
  send(SCI_INDICSETSTYLE, 0, INDIC_ROUNDBOX);
  send(SCI_INDICSETFORE, 0, c2i(_theme->alternateBase()->foreground()));
  send(SCI_SETINDICATORVALUE, 0);
}

void MicroEdit::toggleComment() {
  // Get integer values for start, end selection
  const int selStart = send(SCI_GETSELECTIONSTART), selEnd = send(SCI_GETSELECTIONEND);
  // Convert those positions to line numbers to "expand" the selection to whole lines.
  const int lineStart = send(SCI_LINEFROMPOSITION, selStart), lineEnd = send(SCI_LINEFROMPOSITION, selEnd);

  // Group changes into one undo step
  send(SCI_BEGINUNDOACTION);
  QByteArray buf;
  // Loop through line contents, toggling their comment state. If the line is all whitespace, leave it alone.
  for (int it = lineStart; it <= lineEnd; it++) {
    const int posStart = send(SCI_POSITIONFROMLINE, it), posEnd = send(SCI_GETLINEENDPOSITION, it);
    // Must have extra space for 0-terminator according to docs.
    buf.resize(posEnd - posStart + 1, 0);
    Sci_TextRange tr;
    tr.lpstrText = buf.data();
    tr.chrg.cpMin = posStart, tr.chrg.cpMax = posEnd;
    send(SCI_GETTEXTRANGE, 0, reinterpret_cast<sptr_t>(&tr));
    auto contents = QString::fromLocal8Bit(buf).trimmed();
    static const QString kNul = QString(QChar(u'\0'));
    if (contents.isEmpty() || contents == kNul) continue; // Don't comment out otherwise empty lines
    else if (contents.startsWith("//")) send(SCI_DELETERANGE, posStart, 2);
    else send(SCI_INSERTTEXT, posStart, reinterpret_cast<sptr_t>("//"));
  }
  send(SCI_ENDUNDOACTION);
}
