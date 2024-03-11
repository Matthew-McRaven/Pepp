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
#include "preference.hpp"

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

  void addPreference(QString pref) { preferences_.append(pref); }

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

  Q_PROPERTY(QFont font READ font WRITE setFont NOTIFY fontChanged)
  Q_PROPERTY(qint32 category READ category WRITE setCategory NOTIFY categoryChanged)
  Q_PROPERTY(Preference currentPref READ preference NOTIFY preferenceChanged)

  QHash<int, QByteArray> roleNames_;
  QHash<int, Preference> preferences_;
  QFont font_;
  //Preference* currentPref_ = nullptr;
  qint32 preference_{NormalText};

  QList<Category> categories_;
  qint32 category_{0};

public:

  const Preference preference() const {
    if(preferences_.contains(preference_)) {
      return preferences_.value(preference_);
  }
    else
      return {};
  }

  qint32 category() const { return category_; }
  void setCategory(qint32 category) {
    beginResetModel();
    category_ = category;
    endResetModel();
    emit categoryChanged();
  }

  QFont font() const { return font_; }

  void setFont(QFont font) {
    beginResetModel();
    font_.setFamily(font.family());

    //  Make sure font is at least 8 points
    font_.setPointSize(std::max(font.pointSize(), 8));

    for (auto &it : preferences_) {
      it.setFont(&font_);
    }

    endResetModel();
    emit fontChanged();
  }

  // Define the role names to be used
  enum RoleNames {
    CategoriesRole = Qt::UserRole,
    CurrentCategoryRole,
    CurrentListRole,

    //  Used for identify only
    GeneralRole = Qt::UserRole + 10,
    EditorRole,
    CircuitRole,

    NormalText = GeneralRole * 100, //  Used for iteration.
    Background,
    Selection,
    Test1,

    RowNumber = EditorRole * 100, //  Used for iteration.
    Breakpoint,

    SeqCircuit = CircuitRole * 100, //  Used for iteration.
    CircuitGreen,

    //  Event model for current preference
    CurrentPrefRole,

  };

  Q_ENUM(RoleNames)

  explicit PreferenceModel(QObject *parent = nullptr);
  ~PreferenceModel() = default;

  //	No copying
  PreferenceModel(const PreferenceModel &) = delete;
  PreferenceModel &operator=(const PreferenceModel &) = delete;
  //	No moving
  PreferenceModel(PreferenceModel &&) = delete;
  PreferenceModel &operator=(PreferenceModel &&) = delete;

  //  Data loaded on construction. This reloads
  // void reload();

  // Basic functionality:
  int rowCount(const QModelIndex &parent = {}) const override;
  // int columnCount(const QModelIndex &parent = {}) const override;

  // Fetch data dynamically:
  QVariant data(const QModelIndex &index, int role = CategoriesRole) const override;

  // Editable:
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;

signals:
  void categoryChanged();
  void fontChanged();
  void preferenceChanged();

public slots:
  // void updateTestData();

protected: //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;

private:
  void load();

};

#endif // PREFERENCEMODEL_HPP
