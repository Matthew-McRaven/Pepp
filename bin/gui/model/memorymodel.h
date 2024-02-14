#ifndef MEMORYMODEL_H
#define MEMORYMODEL_H

#include <QAbstractListModel>
#include <QHash>

class MemoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    // Define the role names to be used
    enum RoleNames {
        Byte = Qt::UserRole + 1,
        Character,
    };

    explicit MemoryModel(QObject *parent = nullptr);
    ~MemoryModel() = default;

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

public slots:
    void updateTestData();

//protected:  //  Role Names must be under protected
    QHash<int, QByteArray> roleNames() const override;

private:
    QHash<int, QByteArray>  roleNames_;
    QList<quint8> data_;
};

#endif // MEMORYMODEL_H
