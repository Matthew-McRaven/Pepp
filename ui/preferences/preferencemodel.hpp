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

#ifndef PREFERENCEMODEL_HPP
#define PREFERENCEMODEL_HPP

#include <QAbstractListModel>
#include <QColor>
#include <QFont>
#include <QHash>
#include <QList>
#include "frontend_globals.hpp"

#include "theme.hpp"

class Preference;

class Category {
  QString name_;
  QStringList preferences_;

public:
  Category() = default;
  Category(const QString name) : name_(name){};
  ~Category() = default;

  //	Allow copying
  Category(const Category &) = default;
  Category &operator=(const Category &) = default;
  //	Allow moving
  Category(Category &&) = default;
  Category &operator=(Category &&) = default;

  void addChild(QString pref) { preferences_.append(pref); }

  size_t size() const { return preferences_.size(); }

  const QString preference(int child) const {
    if (child < 0 || child >= size())
      return {};

    return preferences_.at(child);
  }

  QString name() const { return name_; }
};

class FRONTEND_EXPORT PreferenceModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(QFont font READ font     WRITE setFont   NOTIFY fontChanged)
  Q_PROPERTY(Preference* currentPref  READ preference NOTIFY preferenceChanged)
  Q_PROPERTY(qint32 category          READ category   WRITE setCategory NOTIFY categoryChanged)
  Q_PROPERTY(QStringList categoryList MEMBER categoryList_ NOTIFY categoryChanged)

  QHash<int, QByteArray> roleNames_;
  QFont font_;
  Preference* current_;

  QList<Category> categories_;

  //  Returned to QML for category list
  QStringList     categoryList_;
  qint32 category_{0};
  Theme* theme_;

public:

  enum PrefProperty : quint8 {
    Parent = 0,
    Foreground,
    Background,
    Bold,
    Italic,
    Underline,
    Strikeout
  };
  Q_ENUM(PrefProperty)

  // Define the role names to be used
  enum RoleNames : quint32 {
    //CategoriesRole ,
    CurrentCategoryRole = Qt::UserRole,
    CurrentListRole,

    //  Event model for current preference
    CurrentPrefRole,
  };

  Q_ENUM(RoleNames)

  explicit PreferenceModel(Theme* theme, QObject *parent = nullptr);
  ~PreferenceModel() = default;

  //  Call back from QML to updatge preferences
  Q_INVOKABLE void updatePreference(const quint32 key,
                                    const PrefProperty field,
                                    const QVariant& value);

  //	No copying
  PreferenceModel(const PreferenceModel &) = delete;
  PreferenceModel &operator=(const PreferenceModel &) = delete;
  //	No moving
  PreferenceModel(PreferenceModel &&) = delete;
  PreferenceModel &operator=(PreferenceModel &&) = delete;

  //  Getter/setters
  QFont font() const;
  void setFont(QFont font);
  qint32 category() const;
  void setCategory(qint32 category);
  QStringList categoryList();

  // Basic functionality:
  int rowCount(const QModelIndex &parent = {}) const override;

  // Fetch data dynamically:
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // Editable:
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;

  //  Accessor when outside delegate
  Preference* preference() const;

signals:
  void categoryChanged();
  void fontChanged();
  void preferenceChanged();

public slots:

protected: //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;

private:
  void load();

};
#endif // PREFERENCEMODEL_HPP
