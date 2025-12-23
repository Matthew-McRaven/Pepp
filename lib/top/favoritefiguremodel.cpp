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
#include "favoritefiguremodel.hpp"
#include "figure.hpp"
#include "textutils.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"
FavoriteFigureModel::FavoriteFigureModel(QObject *parent) : QAbstractListModel(parent) {
  _registry = helpers::builtins_registry(false);
  auto _6e = helpers::book(6, &*_registry);
  static const std::vector<std::pair<const char *, const char *>> figs = {
      {"05", "03"}, {"05", "06"}, {"05", "07"}, {"05", "10"}};
  for (const auto &[ch, fig] : figs) {
    auto f = _6e->findFigure(QString::fromLatin1(ch), QString::fromLatin1(fig));
    if (f) _figures.append(f);
  }
}

int FavoriteFigureModel::rowCount(const QModelIndex &parent) const { return _figures.size(); }

QVariant FavoriteFigureModel::data(const QModelIndex &index, int role) const {
  using namespace Qt::StringLiterals;
  static const auto rl0 = removeLeading0;
  if (!index.isValid() || index.row() < 0 || index.row() >= _figures.size()) return {};
  auto fig = _figures.at(index.row());
  switch (role) {
  case (int)Roles::FigurePtrRole: return QVariant::fromValue(fig.get());
  case (int)Roles::NameRole: return u"%1.%2"_s.arg(rl0(fig->chapterName()), rl0(fig->figureName()));
  case (int)Roles::TypeRole: return fig->defaultFragmentName();
  case (int)Roles::DescriptionRole: return fig->description();
  }
  return QVariant();
}

QHash<int, QByteArray> FavoriteFigureModel::roleNames() const {
  static auto base = QAbstractListModel::roleNames();
  base[(int)Roles::FigurePtrRole] = "figure";
  base[(int)Roles::NameRole] = "name";
  base[(int)Roles::TypeRole] = "type";
  base[(int)Roles::DescriptionRole] = "description";
  return base;
}
