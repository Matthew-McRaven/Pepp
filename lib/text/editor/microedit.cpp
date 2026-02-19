#include "microedit.hpp"

MicroEdit::MicroEdit(QQuickItem *parent) : EditBase(parent) {}

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

  // Set the selection / highlighting for lines
  send(SCI_SETSELFORE, STYLE_DEFAULT, c2i(_theme->highlight()->foreground()));
  send(SCI_SETSELBACK, STYLE_DEFAULT, c2i(_theme->highlight()->background()));
  // Set the indicator style to a plain underline
  send(SCI_INDICSETSTYLE, 0, INDIC_ROUNDBOX);
  send(SCI_INDICSETFORE, 0, c2i(_theme->alternateBase()->foreground()));
  send(SCI_SETINDICATORVALUE, 0);
}
