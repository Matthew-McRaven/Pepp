#pragma once

#include <QAbstractTableModel>
#include <QtQml/qqmlregistration.h> // Required header for QML_ELEMENT

#include "diagramdata.hpp"
#include "diagramtype.hpp"

class DiagramDataModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

    const int _colSize{32};
    const int _rowSize{32};
    DiagramData _data;

    //  Map to data based on table location
    QMap<DiagramKey, DiagramProperties *> _cells;

    //  Map view to datamodel
    QMap<quint32, DiagramKey> _keys;

    QModelIndex _current{};

    // Sizes in "screen" coordinates
    Q_PROPERTY(QModelIndex currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY
                   currentIndexChanged FINAL)

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

    //  Custom functions accessed by QML
    Q_INVOKABLE void update(const QModelIndex &index);
    Q_INVOKABLE void update(int row, int column);
    Q_INVOKABLE bool clearItemData(const QModelIndexList &indexes);
    Q_INVOKABLE bool clearItemData(const QModelIndex &index) override;
    Q_INVOKABLE DiagramProperties *item(const QModelIndex &index);
    Q_INVOKABLE DiagramProperties *createItem(const QModelIndex &index);
    Q_INVOKABLE QModelIndex index(int row,
                                  int column,
                                  const QModelIndex &parent = {}) const override;

    const QModelIndex currentIndex() const;
    void setCurrentIndex(const QModelIndex);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void currentIndexChanged();
};
