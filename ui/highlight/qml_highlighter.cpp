/*
 * Copyright (c) 2023-2024 J. Stanley Warford, Matthew McRaven
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

#include "./qml_highlighter.hpp"
#include <QTextDocument>
#include <QtQuick/QQuickTextDocument>
#include "./patternedhiglighter.hpp"
#include "./rules_pep_asm.h"
#include "./style.hpp"
#include "./style/map.hpp"
#include "highlight/rules_clike.h"

using namespace highlight;

QMLHighlighter::QMLHighlighter(QObject *parent) : QObject(parent), _highlighter(new PatternedHighlighter(this)) {}

void QMLHighlighter::set_styles(highlight::style::Map *styles) { _styles = styles; }

void QMLHighlighter::set_document(QQuickTextDocument *document) { _highlighter->setDocument(document->textDocument()); }

static const auto _4e = u"Computer Systems, 4th Edition"_qs;
static const auto _5e = u"Computer Systems, 5th Edition"_qs;
static const auto _6e = u"Computer Systems, 6th Edition"_qs;

void QMLHighlighter::set_highlighter(QString edition, QString language) {
  language = language.toLower();
  if (_active.edition == edition && _active.language == language)
    return;

  QList<Rule> _rules = {};
  if (edition.compare(_5e, Qt::CaseInsensitive) == 0) {
    if (language == "pepo") {
    } else if (language == "pep")
      _rules = rules_pep9_asm();
    else if (language == "c")
      _rules = rules_c();
  } else if (edition.compare(_6e, Qt::CaseInsensitive) == 0) {
    if (language == "pepo") {
    } else if (language == "pep")
      _rules = rules_pep10_asm();
    else if (language == "c")
      _rules = rules_c();
  }

  _active.edition = edition;
  _active.language = language;

  QList<PatternedHighlighter::Pattern> _patterns;
  for (const auto &rule : _rules) {
    PatternedHighlighter::Pattern _pattern;
    _pattern.pattern = rule.pattern;
    auto style = _styles->getStyle(rule.style);
    if (style == nullptr)
      continue;
    _pattern.format = style->format();
    _pattern.from = rule.fromState;
    _pattern.to = rule.toState;
    _pattern.reset = rule.reset;
    _patterns.append(_pattern);
  }

  _highlighter->setPatterns(_patterns);
  _highlighter->rehighlight();
}

void QMLHighlighter::clear_highlighter() {
  _highlighter->setPatterns({});
  _highlighter->rehighlight();
}

void QMLHighlighter::on_styles_changed() {
  //_highlighter->se
}
