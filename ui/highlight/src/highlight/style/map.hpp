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

#pragma once
#include <QObject>

#include "../highlight_globals.hpp"
#include "./types.hpp"
#include "../style.hpp"

namespace highlight::style {
class HIGHLIGHT_EXPORT Map : public QObject{
    Q_OBJECT
    Q_PROPERTY(Style* symbol READ getSymbol WRITE setSymbol NOTIFY symbolChanged);
    Q_PROPERTY(Style* comment READ getComment WRITE setComment NOTIFY commentChanged);
    Q_PROPERTY(Style* mnemonic READ getMnemonic WRITE setMnemonic NOTIFY mnemonicChanged);
    Q_PROPERTY(Style* dot READ getDot WRITE setDot NOTIFY dotChanged);
    Q_PROPERTY(Style* quoted READ getQuoted WRITE setQuoted NOTIFY quotedChanged);

public:
    Map(QObject* parent=nullptr);

    ::highlight::Style* getSymbol() const;
    void setSymbol(::highlight::Style* newStyle);

    ::highlight::Style* getComment() const;
    void setComment(::highlight::Style* newStyle);

    ::highlight::Style* getMnemonic() const;
    void setMnemonic(::highlight::Style* newStyle);

    ::highlight::Style* getDot() const;
    void setDot(::highlight::Style* newStyle);

    ::highlight::Style* getQuoted() const;
    void setQuoted(::highlight::Style* newStyle);

    ::highlight::Style* getStyle(Types type) const;
    // returns true if style was changed.
    bool setStyle(Types type, ::highlight::Style* newStyle);
signals:
    void symbolChanged();
    void commentChanged();
    void mnemonicChanged();
    void dotChanged();
    void quotedChanged();

private:
    QMap<Types, ::highlight::Style*> _styles={};
};
};
