/*
 * Copyright (c) 2023 J. Stanley Warford, Matthew McRaven
 *
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
#include "./map.hpp"
#include "../style.hpp"

using namespace highlight::style;
Defaults::Defaults():QObject(){}

void Defaults::pep10_asm(highlight::style::Map * styles)
{
    // new will not leak here, since objects get convert to smart ptr inside Styles.
    using ::highlight::Style;

    Style* mnemonic = new Style(styles);
    mnemonic->setColor(QColor("lightsteelblue"));
    mnemonic->setWeight(QFont::Weight::Bold);
    mnemonic->setItalics(false);
    styles->setMnemonic(mnemonic);

    Style* dot = new Style(styles);
    dot->setColor(QColor("lightsteelblue"));
    dot->setItalics(true);
    styles->setDot(dot);

    Style* symbol = new Style(styles);
    symbol->setColor(QColor(Qt::red).lighter());
    symbol->setWeight(QFont::Weight::Bold);
    styles->setSymbol(symbol);

    Style* comment = new Style(styles);
    comment->setColor(QColor(Qt::green).lighter());
    styles->setComment(comment);

    Style* warning = new Style(styles);
    warning->setBackground(QColor("lightsteelblue"));
    styles->setWarning(warning);

    Style* error = new Style(styles);
    error->setColor(QColor("orangered"));
    styles->setError(error);

}
Q_DECLARE_METATYPE(Defaults)
