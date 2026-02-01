#pragma once

#include <QAbstractTableModel>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "diagramdata.hpp"
#include "diagramtype.hpp"

class DiagramDataModel;

class DiagramDataModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    const int _colSize{26};
    const int _rowSize{1000};
    DiagramData _data;

public:
    enum Role {
        Name = Qt::DisplayRole,
        Id = Qt::UserRole + 1,
        DiagramType,
        ImageSource,
        InputNo,
        OutputNo,
    };

    explicit DiagramDataModel(QObject *parent = nullptr);

    //  Custom functions
    Q_INVOKABLE void update(int row, int column);
    Q_INVOKABLE bool clearItemData(const QModelIndexList &indexes);
    Q_INVOKABLE bool clearItemData(const QModelIndex &index) override;

    // Header:
    /*QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    bool setHeaderData(int section,
                       Qt::Orientation orientation,
                       const QVariant &value,
                       int role = Qt::EditRole) override;*/

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Add data:
    /*bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;
*/
};
