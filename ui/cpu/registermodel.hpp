#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>
#include "./cpu_globals.hpp"

struct CPU_EXPORT RegisterFormatter {
  virtual ~RegisterFormatter() = default;
  virtual QString format() const = 0;
  virtual bool readOnly() const = 0;
  virtual qsizetype length() const = 0;
};

struct CPU_EXPORT TextFormatter : public RegisterFormatter {
  explicit TextFormatter(QString value) : _value(value) {}
  ~TextFormatter() override = default;
  QString format() const override { return _value; }
  bool readOnly() const override { return true; }
  qsizetype length() const override { return _value.length(); }

private:
  QString _value;
};

//  Read only class for change in Register values
class CPU_EXPORT RegisterModel : public QAbstractTableModel {
  Q_OBJECT
  Q_PROPERTY(Roles Box MEMBER _box CONSTANT);
  Q_PROPERTY(Roles RightJustify MEMBER _justify CONSTANT);

public:
  enum class Roles { Box = Qt::UserRole + 1, RightJustify };
  Q_ENUM(Roles)

  explicit RegisterModel(QObject *parent = nullptr);
  ~RegisterModel() = default;

  // Basic functionality:
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
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

protected:  //  Role Names must be under protected
  QHash<int, QByteArray> roleNames() const override;
};
