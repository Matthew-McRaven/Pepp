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

#include "./defaults.hpp"
#include "../style.hpp"
#include "./map.hpp"

using namespace highlight::style;
Defaults::Defaults() : QObject() {}

void Defaults::pep10_asm(highlight::style::Map *styles) {
  // new will not leak here, since objects get convert to smart ptr inside Styles.
  using ::highlight::Style;

  Style *mnemonic = new Style(styles);
  mnemonic->setColor(QColor("lightsteelblue"));
  mnemonic->setWeight(QFont::Weight::Bold);
  mnemonic->setItalics(false);
  styles->setStyle(Types::Mnemonic, mnemonic);

  Style *dot = new Style(styles);
  dot->setColor(QColor("lightsteelblue"));
  dot->setItalics(true);
  styles->setStyle(Types::Dot, dot);

  Style *symbol = new Style(styles);
  symbol->setColor(QColor(Qt::red).lighter());
  symbol->setWeight(QFont::Weight::Bold);
  styles->setStyle(Types::Symbol, symbol);

  Style *comment = new Style(styles);
  comment->setColor(QColor(Qt::green).lighter());
  styles->setStyle(Types::Comment, comment);

  Style *quote = new Style(styles);
  quote->setColor(QColor("orangered"));
  styles->setStyle(Types::Quoted, quote);

  Style *warning = new Style(styles);
  warning->setBackground(QColor("lightsteelblue"));
  styles->setStyle(Types::Warning, warning);

  Style *error = new Style(styles);
  error->setBackground(QColor("orangered"));
  styles->setStyle(Types::Error, error);
}

void Defaults::c(Map *styles) {
  // new will not leak here, since objects get convert to smart ptr inside Styles.
  using ::highlight::Style;

  Style *function = new Style(styles);
  function->setColor(QColor(Qt::red).lighter());
  function->setWeight(QFont::Weight::Bold);
  styles->setStyle(Types::FunctionDec, function);

  Style *decl = new Style(styles);
  decl->setColor(QColor("lightsteelblue"));
  decl->setItalics(true);
  styles->setStyle(Types::OtherKeyword, decl);

  Style *keywords = new Style(styles);
  keywords->setColor(QColor("lightsteelblue"));
  keywords->setWeight(QFont::Weight::Bold);
  styles->setStyle(Types::Keyword, keywords);

  Style *_class = new Style(styles);
  _class->setColor(QColor(Qt::red).lighter());
  _class->setWeight(QFont::Weight::Bold);
  styles->setStyle(Types::Class, _class);

  Style *comment = new Style(styles);
  comment->setColor(QColor(Qt::green).lighter());
  styles->setStyle(Types::Comment, comment);

  Style *quote = new Style(styles);
  quote->setColor("orangered");
  styles->setStyle(Types::Quoted, quote);
}
Q_DECLARE_METATYPE(Defaults)
