#pragma once
#include <QObject>
#include "ScintillaEditBase/ScintillaEditBase.h"
class ScintillaAsmEditBase : public ScintillaEditBase {
  Q_OBJECT
  Q_PROPERTY(QString language READ lexerLanguage WRITE setLexerLanguage NOTIFY lexerLanguageChanged);
  Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY colorChanged);
  Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY colorChanged);
  Q_PROPERTY(QColor errorForegroundColor READ errorForegroundColor WRITE setErrorForegroundColor NOTIFY colorChanged);
  Q_PROPERTY(QColor errorBackgroundColor READ errorBackgroundColor WRITE setErrorBackgroundColor NOTIFY colorChanged);
  Q_PROPERTY(
      QColor commentForegroundColor READ commentForegroundColor WRITE setCommentForegroundColor NOTIFY colorChanged);
  Q_PROPERTY(
      QColor commentBackgroundColor READ commentBackgroundColor WRITE setCommentBackgroundColor NOTIFY colorChanged);
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
  void colorChanged();
  void lineNumbersVisibleChanged();

private:
  int errorStyle = STYLE_LASTPREDEFINED + 1;
  int commentStyle = SCE_PEPASM_COMMENT;
  QString lexerLanguage() const;
  void setLexerLanguage(const QString &language);
  // Must set in CTOR, set*Color. Used in applyStyles().
  int _text, _bg, _errFg, _errBg, _commentFg, _commentBg;
  QColor textColor() const;
  void setTextColor(const QColor &color);
  QColor backgroundColor() const;
  void setBackgroundColor(const QColor &color);
  QColor errorForegroundColor() const;
  void setErrorForegroundColor(const QColor &color);
  QColor errorBackgroundColor() const;
  void setErrorBackgroundColor(const QColor &color);
  QColor commentForegroundColor() const;
  void setCommentForegroundColor(const QColor &color);
  QColor commentBackgroundColor() const;
  void setCommentBackgroundColor(const QColor &color);
  bool lineNumbersVisible() const;
  void setLineNumbersVisible(bool visible);
  // Defer style update so that we can layer multiple changes over defaults.
  void applyStyles();
};
