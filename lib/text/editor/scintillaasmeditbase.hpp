#pragma once
#include <QObject>
#include "ScintillaEditBase/ScintillaEditBase.h"
#include "settings/palette.hpp"

class ScintillaEditBaseExporter : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(ScintillaEditBase)
  QML_FOREIGN(ScintillaEditBase)
};

class ScintillaAsmEditBase : public ScintillaEditBase {
  Q_OBJECT
  Q_PROPERTY(QString language READ lexerLanguage WRITE setLexerLanguage NOTIFY lexerLanguageChanged);
  Q_PROPERTY(pepp::settings::Palette *theme READ theme WRITE setTheme NOTIFY themeChanged)
  Q_PROPERTY(
      bool lineNumbersVisible READ lineNumbersVisible WRITE setLineNumbersVisible NOTIFY lineNumbersVisibleChanged);
  Q_PROPERTY(int caretBlink READ caretBlink WRITE setCaretBlink NOTIFY caretBlinkChanged);
  QML_ELEMENT

public:
  enum class Action { ToggleBP, AddBP, RemoveBP, ScrollTo, HighlightExclusive, MakeConditional, MakeUnconditional };
  Q_ENUM(Action);
  ScintillaAsmEditBase(QQuickItem *parent = 0);
public slots:
  // For errors
  void clearAllEOLAnnotations();
  void setEOLAnnotationsVisible(int style);
  void addEOLAnnotation(int line, const QString &annotation);
  // For listing
  void clearAllInlineAnnotations();
  void setInlineAnnotationsVisible(int style);
  void addInlineAnnotation(int line, const QString &annotation);
  // Breakpoints & folding
  void onMarginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin);
  void onLineAction(int line, Action action);
  void onClearAllBreakpoints();
  void onRequestAllBreakpoints();
  void applyStyles();

  Q_INVOKABLE void cut();
  Q_INVOKABLE void copy();
  Q_INVOKABLE void paste();
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
signals:
  void lexerLanguageChanged();
  void themeChanged();
  void lineNumbersVisibleChanged();
  void modifyLine(int line, Action action);
  void caretBlinkChanged();

private:
  const int errorStyle = STYLE_LASTPREDEFINED + 1;
  const int warningStyle = STYLE_LASTPREDEFINED + 2;
  const int symbolStyle = SCE_PEPASM_SYMBOL_DECL;
  const int mnemonicStyle = SCE_PEPASM_MNEMONIC;
  const int directiveStyle = SCE_PEPASM_DIRECTIVE;
  const int macroStyle = SCE_PEPASM_MACRO;
  const int charStyle = SCE_PEPASM_CHARACTER;
  const int stringStyle = SCE_PEPASM_STRING;
  const int commentStyle = SCE_PEPASM_COMMENT;

  const int BPStyle = 1;
  const int BPStyleMask = 1 << BPStyle;
  const int conditionalBPStyle = 2;
  const int conditionalBPStyleMask = 1 << conditionalBPStyle;
  QString lexerLanguage() const;
  void setLexerLanguage(const QString &language);
  pepp::settings::Palette *_theme = nullptr;
  pepp::settings::Palette *theme() const;
  void setTheme(pepp::settings::Palette *theme);

  bool lineNumbersVisible() const;
  void setLineNumbersVisible(bool visible);

  int caretBlink() const;
  void setCaretBlink(int blink);
  // Defer style update so that we can layer multiple changes over defaults.
};
