#pragma once
#include <QObject>
#include "ScintillaEditBase/ScintillaEditBase.h"
#include "preferences/theme.hpp"
class ScintillaAsmEditBase : public ScintillaEditBase {
  Q_OBJECT
  Q_PROPERTY(QString language READ lexerLanguage WRITE setLexerLanguage NOTIFY lexerLanguageChanged);
  Q_PROPERTY(Theme *theme READ theme WRITE setTheme NOTIFY themeChanged)
  Q_PROPERTY(
      bool lineNumbersVisible READ lineNumbersVisible WRITE setLineNumbersVisible NOTIFY lineNumbersVisibleChanged);

public:
  ScintillaAsmEditBase(QQuickItem *parent = 0);
public slots:
  // For errors
  void clearAllEOLAnnotations();
  void setEOLAnnotationsVisibile(int style);
  void addEOLAnnotation(int line, const QString &annotation);
  // For breakpoints
  void onMarginClicked(Scintilla::Position position, Scintilla::KeyMod modifiers, int margin);
signals:
  void lexerLanguageChanged();
  void themeChanged();
  void lineNumbersVisibleChanged();

private:
  const int errorStyle = STYLE_LASTPREDEFINED + 1;
  const int commentStyle = SCE_PEPASM_COMMENT;
  QString lexerLanguage() const;
  void setLexerLanguage(const QString &language);
  Theme *_theme = nullptr;
  Theme *theme() const;
  void setTheme(Theme *theme);

  bool lineNumbersVisible() const;
  void setLineNumbersVisible(bool visible);
  // Defer style update so that we can layer multiple changes over defaults.
  void applyStyles();
};
