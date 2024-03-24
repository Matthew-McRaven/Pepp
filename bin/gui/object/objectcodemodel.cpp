/*
 * Copyright (c) 2024 J. Stanley Warford, Matthew McRaven
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

#include "objectcodemodel.hpp"

ObjectCodeModel::ObjectCodeModel(QObject *parent) : QAbstractTableModel(parent) {}

int ObjectCodeModel::rowCount(const QModelIndex &parent) const { return 0; }

int ObjectCodeModel::columnCount(const QModelIndex &parent) const { return 0; }

QVariant ObjectCodeModel::data(const QModelIndex &index, int role) const { return {}; }

bool ObjectCodeModel::setData(const QModelIndex &index, const QVariant &value, int role) { return false; }

Qt::ItemFlags ObjectCodeModel::flags(const QModelIndex &index) const { return {}; }

bool ObjectCodeModel::insertRows(int row, int count, const QModelIndex &parent) { return false; }

bool ObjectCodeModel::removeRows(int row, int count, const QModelIndex &parent) { return false; }
