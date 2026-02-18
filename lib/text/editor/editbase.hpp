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
#pragma once
#include <QObject>
#include "ScintillaEditBase/ScintillaEditBase.h"
#include "settings/palette.hpp"

class ScintillaEditBaseExporter : public QObject {
  Q_OBJECT
  QML_NAMED_ELEMENT(ScintillaEditBase)
  QML_FOREIGN(ScintillaEditBase)
};

sptr_t c2i(const QColor &color);
// Editor ignores alpha, so we need do do alpha compositing ourselves
// See: https://en.wikipedia.org/wiki/Alpha_compositing
sptr_t alphaBlend(const QColor &front, const QColor &back);

class EditBase : public ScintillaEditBase {
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
  EditBase(QQuickItem *parent = 0);
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
  virtual void applyStyles();

  Q_INVOKABLE void cut();
  Q_INVOKABLE void copy();
  Q_INVOKABLE void paste();
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  Q_INVOKABLE void selectAll();
signals:
  void lexerLanguageChanged();
  void themeChanged();
  void lineNumbersVisibleChanged();
  void modifyLine(int line, Action action);
  void caretBlinkChanged();

protected:
  const int errorStyle = STYLE_LASTPREDEFINED + 1;
  const int warningStyle = STYLE_LASTPREDEFINED + 2;
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
};
