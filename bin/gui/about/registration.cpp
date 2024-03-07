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

#include "registration.hpp"
#include "help/about/pepp.hpp"
#include "help/about/version.hpp"

Version::Version(QObject *parent) : QObject(parent) {}
QString Version::git_sha() { return about::g_GIT_SHA1(); }
QString Version::git_tag() { return about::g_GIT_TAG(); }
bool Version::git_dirty() { return about::g_GIT_LOCAL_CHANGES(); }
int Version::version_major() { return about::g_MAJOR_VERSION(); }
int Version::version_minor() { return about::g_MINOR_VERSION(); }
int Version::version_patch() { return about::g_PATCH_VERSION(); }
QString Version::version_str_full() {
  return u"%1.%2.%3"_qs.arg(version_major()).arg(version_minor()).arg(version_patch());
}

Maintainer::Maintainer(QString name, QString email, QObject *parent) : QObject(parent), _name(name), _email(email) {}
QString Maintainer::name() { return _name; }
QString Maintainer::email() { return _email; }

namespace about {
void registerTypes(QQmlApplicationEngine &engine) {
  qmlRegisterSingletonType<Version>("edu.pepp", 1, 0, "Version",
                                    [](QQmlEngine *, QJSEngine *) { return new Version(); });
  qmlRegisterUncreatableType<Maintainer>("edu.pepp", 1, 0, "Maintainer", "Must be created from C++");
  qmlRegisterSingletonType<QList<Maintainer *>>("edu.pepp", 1, 0, "Maintainers", [](QQmlEngine *, QJSEngine *) {
    // Need global scope ::, or it picks up about::Maintainer
    QList<::Maintainer *> maintainers{};
    for (const auto &maintainer : about::maintainers()) {
      auto *item = new ::Maintainer(maintainer.name, maintainer.email, nullptr);
      maintainers.push_back(item);
    }
    // Class assumes ownership of objects via modifying parent pointer.
    auto owning = new MaintainerList(maintainers);
    return owning;
  });
}
} // namespace about
MaintainerList::MaintainerList(QList<Maintainer *> list, QObject *parent) : QAbstractListModel(parent), _list(list) {
  // Must re-parent to avoid memory leaks.
  for (auto *item : list)
    item->setParent(this);
}

int MaintainerList::rowCount(const QModelIndex &parent) const { return _list.size(); }

QVariant MaintainerList::data(const QModelIndex &index, int role) const {
  int row = index.row();
  if (!index.isValid() || row < 0 || row >= _list.size())
    return QVariant();
  auto *item = _list.at(row);
  switch (role) {
  case NAME:
    return item->name();
  case EMAIL:
    return item->email();
  default:
    return QVariant();
  }
}

QHash<int, QByteArray> MaintainerList::roleNames() const {
  QHash<int, QByteArray> roles;
  roles[NAME] = "name";
  roles[EMAIL] = "email";
  return roles;
}
