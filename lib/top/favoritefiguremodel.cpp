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
#include "core/resources/figures/builtin_registry.hpp"
#include "core/resources/figures/figure.hpp"
#include "textutils.hpp"
#include "toolchain/helpers/assemblerregistry.hpp"

#include <QQmlEngine>
FavoriteFigureModel::FavoriteFigureModel(QObject *parent) : QAbstractListModel(parent) {
  _registry = helpers::builtins_registry(false);
  _favoritesCategory = pepp::settings::detail::AppSettingsData::getInstance()->favorites();
  connect(_favoritesCategory, &pepp::settings::FavoriteFigureCategory::favoritesChanged, this,
          &FavoriteFigureModel::onFavoritesChanged);
  onFavoritesChanged();
}

int FavoriteFigureModel::rowCount(const QModelIndex &parent) const { return _figures.size(); }

QVariant FavoriteFigureModel::data(const QModelIndex &index, int role) const {
  using namespace Qt::StringLiterals;
  if (!index.isValid() || index.row() < 0 || index.row() >= _figures.size()) return {};
  auto &wrapped_fig = _figures.at(index.row());
  auto fig = wrapped_fig->underlying();
  switch (role) {
  case (int)Roles::FigurePtrRole: return QVariant::fromValue(wrapped_fig.get());
  case (int)Roles::NameRole:
    return u"%1.%2"_s.arg(removeLeading0(QString::fromStdString(fig->name_chapter())),
                          removeLeading0(QString::fromStdString(fig->name_figure())));
  case (int)Roles::TypeRole: return QString::fromStdString(fig->default_fragment_name());
  case (int)Roles::DescriptionRole: return QString::fromStdString(fig->description());
  case (int)Roles::EditionRole: {
    if (auto book = fig->book().lock(); !book) return {};
    else {
      auto name = book->name();
      auto edition = pepp::edition_number(name);
      return u"%1th Edition"_s.arg(edition);
    }
  }
  }
  return QVariant();
}

QHash<int, QByteArray> FavoriteFigureModel::roleNames() const {
  static auto base = QAbstractListModel::roleNames();
  base[(int)Roles::FigurePtrRole] = "figure";
  base[(int)Roles::NameRole] = "name";
  base[(int)Roles::TypeRole] = "type";
  base[(int)Roles::DescriptionRole] = "description";
  base[(int)Roles::EditionRole] = "edition";
  return base;
}

void FavoriteFigureModel::add_figure(builtins::FigureWrapper *figure) { _favoritesCategory->addFavorite(figure); }

void FavoriteFigureModel::onFavoritesChanged() {
  beginResetModel();
  _figures.clear();
  auto favorites = _favoritesCategory->favorites();
  for (const auto &fav : std::as_const(favorites)) {
    if (auto book = helpers::book(fav.edition(), &*_registry); !book) continue;
    else if (auto f = book->find_figure(fav.chapter().toStdString(), fav.figure().toStdString()); f) {
      auto fw = std::make_unique<builtins::FigureWrapper>(f);
      QQmlEngine::setObjectOwnership(fw.get(), QQmlEngine::CppOwnership);
      _figures.push_back(std::move(fw));
    }
  }
  std::sort(_figures.begin(), _figures.end());
  endResetModel();
}
