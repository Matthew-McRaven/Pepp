#include "asmedit.hpp"

AsmEdit::AsmEdit(QQuickItem *parent) : EditBase(parent) {
  // Margin 0 already used for line numbers, so use margin 1 for breakpoints.
  send(SCI_SETMARGINSENSITIVEN, 1, true);
  send(SCI_SETMARGINSENSITIVEN, 2, true);
  // For code folding of comments and macros
  send(SCI_SETMARGINWIDTHN, 2, getCharWidth() * 2);
  send(SCI_SETMARGINTYPEN, 2, SC_MARGIN_SYMBOL);
  send(SCI_SETMARGINMASKN, 2, SC_MASK_FOLDERS);
}

void AsmEdit::applyStyles() {
  // WARNING: If you anticipate a color having an alpha value, you will need to do the blending yourself!
  if (_theme == nullptr) return;
  auto baseBack = _theme->base()->background();
  send(SCI_STYLESETFORE, STYLE_DEFAULT, c2i(_theme->base()->foreground()));
  send(SCI_SETCARETFORE, c2i(_theme->base()->foreground()));
  setStylesFont(_theme->baseMono()->font(), STYLE_DEFAULT);
  send(SCI_STYLESETBACK, STYLE_DEFAULT, c2i(_theme->base()->background()));
  send(SCI_STYLECLEARALL, 0, 0);
  auto macroFont = [&](const pepp::settings::PaletteItem *item) {
    auto asEditorItem = qobject_cast<const pepp::settings::EditorPaletteItem *>(item);
    if (asEditorItem) {
      auto f = asEditorItem->macroFont();
      f.setItalic(true);
      return f;
    }
    return pepp::settings::default_mono();
  };

  for (int mask : {0, SCE_PEPASM_DEFAULT_GEN}) {
    if (mask) {
      setStylesFont(macroFont(_theme->mnemonic()), mask);
      setStylesFont(macroFont(_theme->mnemonic()), SCE_PEPASM_IDENTIFIER | mask);
    }

    send(SCI_STYLESETFORE, symbolStyle | mask, c2i(_theme->symbol()->foreground()));
    send(SCI_STYLESETITALIC, symbolStyle | mask, _theme->symbol()->font().italic());
    send(SCI_STYLESETBOLD, symbolStyle | mask, _theme->symbol()->font().bold());
    send(SCI_STYLESETBACK, symbolStyle | mask, alphaBlend(_theme->symbol()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->symbol()), symbolStyle | mask);

    send(SCI_STYLESETFORE, mnemonicStyle | mask, c2i(_theme->mnemonic()->foreground()));
    send(SCI_STYLESETITALIC, mnemonicStyle | mask, _theme->mnemonic()->font().italic());
    send(SCI_STYLESETBOLD, mnemonicStyle | mask, _theme->mnemonic()->font().bold());
    send(SCI_STYLESETBACK, mnemonicStyle | mask, alphaBlend(_theme->mnemonic()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->mnemonic()), mnemonicStyle | mask);

    send(SCI_STYLESETFORE, directiveStyle | mask, c2i(_theme->directive()->foreground()));

    send(SCI_STYLESETBOLD, directiveStyle | mask, _theme->directive()->font().bold());
    send(SCI_STYLESETBACK, directiveStyle | mask, alphaBlend(_theme->directive()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->directive()), directiveStyle | mask);

    send(SCI_STYLESETFORE, macroStyle | mask, c2i(_theme->macro()->foreground()));
    send(SCI_STYLESETITALIC, macroStyle | mask, _theme->macro()->font().italic());
    send(SCI_STYLESETBOLD, macroStyle | mask, _theme->macro()->font().bold());
    send(SCI_STYLESETBACK, macroStyle | mask, alphaBlend(_theme->macro()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->macro()), macroStyle | mask);

    send(SCI_STYLESETFORE, charStyle | mask, c2i(_theme->character()->foreground()));
    send(SCI_STYLESETITALIC, charStyle | mask, _theme->character()->font().italic());
    send(SCI_STYLESETBOLD, charStyle | mask, _theme->character()->font().bold());
    send(SCI_STYLESETBACK, charStyle | mask, alphaBlend(_theme->character()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->character()), charStyle | mask);

    send(SCI_STYLESETFORE, stringStyle | mask, c2i(_theme->string()->foreground()));
    send(SCI_STYLESETITALIC, stringStyle | mask, _theme->string()->font().italic());
    send(SCI_STYLESETBOLD, stringStyle | mask, _theme->string()->font().bold());
    send(SCI_STYLESETBACK, stringStyle | mask, alphaBlend(_theme->string()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->string()), stringStyle | mask);

    send(SCI_STYLESETFORE, commentStyle | mask, c2i(_theme->comment()->foreground()));
    send(SCI_STYLESETITALIC, commentStyle | mask, _theme->comment()->font().italic());
    send(SCI_STYLESETBOLD, commentStyle | mask, _theme->comment()->font().bold());
    send(SCI_STYLESETBACK, commentStyle | mask, alphaBlend(_theme->comment()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->comment()), commentStyle | mask);

    send(SCI_STYLESETFORE, SCE_PEPASM_COMMENT_LINE | mask, c2i(_theme->comment()->foreground()));
    send(SCI_STYLESETITALIC, SCE_PEPASM_COMMENT_LINE | mask, _theme->comment()->font().italic());
    send(SCI_STYLESETBOLD, SCE_PEPASM_COMMENT_LINE | mask, _theme->comment()->font().bold());
    send(SCI_STYLESETBACK, SCE_PEPASM_COMMENT_LINE | mask, alphaBlend(_theme->comment()->background(), baseBack));
    if (mask) setStylesFont(macroFont(_theme->comment()), SCE_PEPASM_COMMENT_LINE | mask);
  }

  send(SCI_STYLESETFORE, errorStyle, c2i(_theme->error()->foreground()));
  send(SCI_STYLESETITALIC, errorStyle, _theme->error()->font().italic());
  send(SCI_STYLESETBOLD, errorStyle, _theme->error()->font().bold());
  send(SCI_STYLESETBACK, errorStyle, alphaBlend(_theme->error()->background(), baseBack));

  send(SCI_STYLESETFORE, SCE_PEPASM_MACRO_START, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, SCE_PEPASM_MACRO_START, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, SCE_PEPASM_MACRO_START, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, SCE_PEPASM_MACRO_START, alphaBlend(_theme->comment()->background(), baseBack));
  setStylesFont(macroFont(_theme->comment()), SCE_PEPASM_MACRO_START);

  send(SCI_STYLESETFORE, SCE_PEPASM_MACRO_END, c2i(_theme->comment()->foreground()));
  send(SCI_STYLESETITALIC, SCE_PEPASM_MACRO_END, _theme->comment()->font().italic());
  send(SCI_STYLESETBOLD, SCE_PEPASM_MACRO_END, _theme->comment()->font().bold());
  send(SCI_STYLESETBACK, SCE_PEPASM_MACRO_END, alphaBlend(_theme->comment()->background(), baseBack));
  setStylesFont(macroFont(_theme->comment()), SCE_PEPASM_MACRO_END);

  send(SCI_MARKERSETFORE, BPStyle, c2i(_theme->error()->foreground()));
  send(SCI_MARKERSETBACK, BPStyle, c2i(_theme->error()->background()));
  send(SCI_MARKERSETFORE, conditionalBPStyle, c2i(_theme->error()->background()));
  send(SCI_MARKERSETBACK, conditionalBPStyle, c2i(_theme->error()->foreground()));

  // Ensure fold margin is tied to the theme. Must set normal+hi else checkerboard ensues.
  send(SCI_SETFOLDMARGINCOLOUR, true, c2i(_theme->midlight()->background()));
  send(SCI_SETFOLDMARGINHICOLOUR, true, c2i(_theme->midlight()->background()));
  // Set the selection / highlighting for lines
  send(SCI_SETSELFORE, STYLE_DEFAULT, c2i(_theme->highlight()->foreground()));
  send(SCI_SETSELBACK, STYLE_DEFAULT, c2i(_theme->highlight()->background()));
  // Set the indicator style to a plain underline
  send(SCI_INDICSETSTYLE, 0, INDIC_ROUNDBOX);
  send(SCI_INDICSETFORE, 0, c2i(_theme->alternateBase()->foreground()));
  send(SCI_SETINDICATORVALUE, 0);
}
