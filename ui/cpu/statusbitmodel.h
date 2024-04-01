#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>
#include "./cpu_global.hpp"

//  For testing only
#include <QRandomGenerator>

class CPU_EXPORT StatusBit {
  Q_GADGET
  Q_PROPERTY(QString statusBit READ statusBit WRITE setStatusBit)
  Q_PROPERTY(bool flag READ flag WRITE setFlag)

  QString statusBit_;
  bool flag_ = false;

public:
    StatusBit() = default;
    StatusBit(const QString statusBit, const bool flag );
    ~StatusBit()= default;

    //	Disallow copying
    StatusBit( const StatusBit& ) = default;
    StatusBit& operator=( const StatusBit& ) = default;
    //	Moving OK
    StatusBit( StatusBit&& ) noexcept = default;
    StatusBit& operator=( StatusBit&& ) = default;

    //  Getter & Setter
    QString statusBit() const {return statusBit_;}
    bool flag() const {return flag_;}

    void setStatusBit( const QString statusBit){ statusBit_ = statusBit;}
    void setFlag( const bool flag){ flag_ = flag;}
};

//  Read only class for change in status bits
class CPU_EXPORT StatusBitModel : public QAbstractListModel {
  Q_OBJECT

public:
    // Define the role names to be used
    enum RoleNames {
        Status = Qt::UserRole + 1,
        Flag,
    };

    explicit StatusBitModel(QObject *parent = nullptr);
    ~StatusBitModel() = default;

    //  Data loaded on construction. This reloads
    void reload();

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // Fetch data dynamically:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

public slots:
    void updateTestData();

protected:  //  Role Names must be under protected
    QHash<int, QByteArray> roleNames() const override;

private:
    void load();

    QHash<int, QByteArray>  roleNames_;
    QList<StatusBit>        statusBits_;
};
