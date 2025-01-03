#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>
#include "api2/trace/iterator.hpp"

struct RegisterFormatter;
//  Read only class for change in Register values
class RegisterModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(Roles Box MEMBER _box CONSTANT);
  Q_PROPERTY(Roles RightJustify MEMBER _justify CONSTANT);
  Q_PROPERTY(Roles Choices MEMBER _choices CONSTANT);
  Q_PROPERTY(Roles Selected MEMBER _selected CONSTANT);
  QML_ELEMENT

public:
  enum class Roles { Box = Qt::UserRole + 1, RightJustify, Choices, Selected };
  Q_ENUM(Roles)

  explicit RegisterModel(QObject *parent = nullptr);
  ~RegisterModel() = default;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  // Append rows / columns to data model.
  void appendFormatters(QVector<QSharedPointer<RegisterFormatter>> formatters);
  Q_INVOKABLE qsizetype columnCharWidth(int column) const;
public slots:
  void onUpdateGUI();

private:
  uint32_t _cols = 0;
  QVector<QVector<QSharedPointer<RegisterFormatter>>> _data;
  const Roles _box = Roles::Box;
  const Roles _justify = Roles::RightJustify;
  const Roles _choices = Roles::Choices;
  const Roles _selected = Roles::Selected;

protected: //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;
};
