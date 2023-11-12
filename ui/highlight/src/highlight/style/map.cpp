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

#include <QQmlEngine>

#include "./map.hpp"

highlight::style::Map::Map(QObject *parent): QObject(parent)
{

}

::highlight::Style* highlight::style::Map::getSymbol() const
{
    return _styles[Types::Symbol];
}

void highlight::style::Map::setSymbol(::highlight::Style* newStyle)
{
    if(setStyle(Types::Symbol, newStyle)) emit symbolChanged();
}

::highlight::Style* highlight::style::Map::getComment() const
{
    return _styles[Types::Comment];
}

void highlight::style::Map::setComment(::highlight::Style *newStyle)
{
    if(setStyle(Types::Comment, newStyle)) emit commentChanged();
}

::highlight::Style* highlight::style::Map::getMnemonic() const
{
    return _styles[Types::Mnemonic];
}

void highlight::style::Map::setMnemonic(::highlight::Style *newStyle)
{
    if(setStyle(Types::Mnemonic, newStyle)) emit mnemonicChanged();
}

::highlight::Style* highlight::style::Map::getDot() const
{
    return _styles[Types::Dot];
}

void highlight::style::Map::setDot(::highlight::Style *newStyle)
{
    if(setStyle(Types::Dot, newStyle)) emit dotChanged();
}

::highlight::Style* highlight::style::Map::getQuoted() const
{
    return _styles[Types::Quoted];
}

void highlight::style::Map::setQuoted(::highlight::Style *newStyle)
{
    if(setStyle(Types::Quoted, newStyle)) emit quotedChanged();
}

::highlight::Style *highlight::style::Map::getStyle(Types type) const
{
    return _styles[type];
}

bool highlight::style::Map::setStyle(highlight::style::Types key, ::highlight::Style *newStyle)
{
    // Keys initialized to nullptr; avoid dereference for member-wise equality comparison.
    if(_styles[key]==nullptr);
    else if(*newStyle == *_styles[key]) return false;
    // We prevsiously claimed ownership of the styles; we must delete them.
    if(auto _old = _styles[key]; _old) delete _old;
    _styles[key] = newStyle;
    // We can receive objects from both C++ and QML. These have different lifetime semantics.
    // We transfer ownership of QML objects to C++ so that there's only one place it can be reclaimed,
    // and the remove its parent to prevent a double-free.
    QQmlEngine::setObjectOwnership(newStyle, QQmlEngine::CppOwnership);
    newStyle->setParent(nullptr);
    return true;
}

Q_DECLARE_METATYPE(highlight::style::Map*)
